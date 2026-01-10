#include "si/shell/executor.hpp"
#include "si/foundation/logging.hpp"
#include "si/shell/block_manager.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

namespace si::shell {

CommandExecutor::CommandExecutor() {}
CommandExecutor::~CommandExecutor() {}

ExecutionResult CommandExecutor::execute(const std::string &command) {
  ExecutionResult result;

  // Create pipes for stdout and stderr
  int stdout_pipe[2];
  int stderr_pipe[2];

  if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
    SI_LOG_ERROR("Failed to create pipes");
    return result;
  }

  pid_t pid = fork();

  if (pid < 0) {
    SI_LOG_ERROR("Failed to fork process");
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    return result;
  }

  if (pid == 0) {
    // Child process
    close(stdout_pipe[0]); // Close read end
    close(stderr_pipe[0]);

    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    execl("/bin/bash", "bash", "-c", command.c_str(), nullptr);
    _exit(127); // exec failed
  }

  // Parent process
  close(stdout_pipe[1]); // Close write end
  close(stderr_pipe[1]);

  // Read stdout
  std::array<char, 4096> buffer;
  ssize_t n;
  while ((n = read(stdout_pipe[0], buffer.data(), buffer.size())) > 0) {
    result.stdout_output.append(buffer.data(), n);
  }
  close(stdout_pipe[0]);

  // Read stderr
  while ((n = read(stderr_pipe[0], buffer.data(), buffer.size())) > 0) {
    result.stderr_output.append(buffer.data(), n);
  }
  close(stderr_pipe[0]);

  // Wait for child
  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else {
    result.exit_code = -1;
  }

  result.success = (result.exit_code == 0);
  return result;
}

#include <pty.h>
#include <termios.h>
#include <utmp.h>

int CommandExecutor::execute_stream(
    const std::string &command, const std::string &cwd,
    const std::string &shell,
    std::function<void(const std::string &)> on_stdout,
    std::function<void(const std::string &)> on_stderr, int cols, int rows) {

  SI_LOG_INFO("EXECUTOR: execute_stream called: {} in {} ({}x{})", command, cwd,
              cols, rows);

  struct winsize ws;
  ws.ws_col = (cols > 0) ? (unsigned short)cols : 80;
  ws.ws_row = (rows > 0) ? (unsigned short)rows : 24;
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  int master_fd;
  pid_t pid = forkpty(&master_fd, nullptr, nullptr, &ws);

  if (pid < 0) {
    SI_LOG_ERROR("Failed to forkpty for streaming");
    return -1;
  }

  if (pid == 0) {
    // Child process
    // Set working directory
    if (!cwd.empty() && cwd != ".") {
      if (chdir(cwd.c_str()) != 0) {
        perror("chdir");
      }
    }

    // Set environment to ensure colors and TTY features work
    setenv("TERM", "xterm-256color", 1);
    setenv("LANG", "en_US.UTF-8", 1);
    setenv("CLICOLOR", "1", 1);
    setenv("FORCE_COLOR", "1", 1);

    // Invoke shell as interactive to load aliases (.bashrc)
    // We use -i for interactive mode and -c for the command.
    // Note: Some shells might need -l or special handling for non-tty parents,
    // but forkpty provides a real TTY.
    if (shell.find("bash") != std::string::npos) {
      execl(shell.c_str(), "bash", "-i", "-c", command.c_str(), nullptr);
    } else {
      execl(shell.c_str(), shell.c_str(), "-c", command.c_str(), nullptr);
    }

    _exit(127);
  }

  // Parent process
  std::array<char, 4096> buffer;
  ssize_t n;

  // With PTY, stdout and stderr are merged into the master_fd.
  // We prioritize on_stdout for all terminal output.
  while ((n = read(master_fd, buffer.data(), buffer.size())) > 0) {
    if (on_stdout) {
      on_stdout(std::string(buffer.data(), n));
    }
  }

  close(master_fd);

  int status;
  waitpid(pid, &status, 0);

  int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  SI_LOG_INFO("EXECUTOR: Command finished, exit code: {}", exit_code);
  return exit_code;
}

int CommandExecutor::run(const std::string &command) {
  return execute_stream(
      command, ".", "/bin/bash",
      [](const std::string &s) { std::cout << s << std::flush; },
      [](const std::string &s) { std::cerr << s << std::flush; }, 80, 24);
}

int CommandExecutor::execute_to_block(const std::string &block_id,
                                      const std::string &command,
                                      const std::string &cwd,
                                      const std::string &shell, int cols,
                                      int rows) {
  // Use execute_stream but route output to BlockManager

  SI_LOG_INFO("EXECUTOR: execute_to_block called: {} cmd: {} ({}x{})", block_id,
              command, cols, rows);

  auto &bm = BlockManager::instance();

  int exit_code = execute_stream(
      command, cwd, shell,
      [&bm, &block_id](const std::string &s) {
        bm.append_output(block_id, s, "stdout");
      },
      [&bm, &block_id](const std::string &s) {
        bm.append_output(block_id, s, "stderr");
      },
      cols, rows);

  bm.complete_block(block_id, exit_code);
  return exit_code;
}

} // namespace si::shell
