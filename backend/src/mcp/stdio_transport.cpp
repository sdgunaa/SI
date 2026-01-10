#include "si/mcp/stdio_transport.hpp"
#include "si/foundation/logging.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

namespace si::mcp {

StdioTransport::StdioTransport(const std::string &command,
                               const std::vector<std::string> &args)
    : command_(command), args_(args) {}

StdioTransport::~StdioTransport() { close(); }

bool StdioTransport::start() {
  if (running_)
    return true;

  int p_stdin[2], p_stdout[2], p_stderr[2];

  if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0 || pipe(p_stderr) != 0) {
    SI_LOG_ERROR("Failed to create pipes for MCP transport");
    return false;
  }

  pid_ = fork();
  if (pid_ < 0) {
    SI_LOG_ERROR("Failed to fork MCP process");
    ::close(p_stdin[0]);
    ::close(p_stdin[1]);
    ::close(p_stdout[0]);
    ::close(p_stdout[1]);
    ::close(p_stderr[0]);
    ::close(p_stderr[1]);
    return false;
  }

  if (pid_ == 0) {
    // Child process
    dup2(p_stdin[0], STDIN_FILENO);
    dup2(p_stdout[1], STDOUT_FILENO);
    dup2(p_stderr[1], STDERR_FILENO);

    // Close all original fd
    ::close(p_stdin[0]);
    ::close(p_stdin[1]);
    ::close(p_stdout[0]);
    ::close(p_stdout[1]);
    ::close(p_stderr[0]);
    ::close(p_stderr[1]);

    // Prepare args
    std::vector<char *> c_args;
    c_args.push_back(strdup(command_.c_str()));
    for (const auto &s : args_)
      c_args.push_back(strdup(s.c_str()));
    c_args.push_back(nullptr);

    execvp(command_.c_str(), c_args.data());
    perror("mcp execvp failed");
    _exit(1);
  }

  // Parent process
  ::close(p_stdin[0]);
  stdin_fd_ = p_stdin[1];
  ::close(p_stdout[1]);
  stdout_fd_ = p_stdout[0];
  ::close(p_stderr[1]);
  stderr_fd_ = p_stderr[0];

  running_ = true;
  read_thread_ = std::thread(&StdioTransport::read_loop, this);

  // Stderr thread
  err_thread_ = std::thread([this]() {
    char buffer[1024];
    ssize_t n;
    while (running_ && stderr_fd_ != -1) {
      n = ::read(stderr_fd_, buffer, sizeof(buffer) - 1);
      if (n > 0) {
        buffer[n] = 0;
        std::string s(buffer);
        // Remove trailing newline
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
          s.pop_back();
        if (!s.empty())
          SI_LOG_WARN("[MCP {} stderr] {}", command_, s);
      } else {
        break;
      }
    }
  });

  return true;
}

void StdioTransport::close() {
  bool was_running = running_.exchange(false);
  if (!was_running)
    return;

  if (stdin_fd_ != -1) {
    ::close(stdin_fd_);
    stdin_fd_ = -1;
  }
  if (stdout_fd_ != -1) {
    ::close(stdout_fd_);
    stdout_fd_ = -1;
  }
  // stderr_fd_ closed by thread loop logic usually or here?
  // Close stdout/stderr fd to wake up blocking reads if using select/poll, but
  // here we use blocking read. Threads might be blocked on read(). Closing FDs
  // from another thread is UB or behavior specific. Best way is to signal or
  // just closing them usually causes read to return error.
  if (stderr_fd_ != -1) {
    ::close(stderr_fd_);
    stderr_fd_ = -1;
  }

  if (read_thread_.joinable())
    read_thread_.join();
  if (err_thread_.joinable())
    err_thread_.join();

  if (pid_ != -1) {
    int status;
    waitpid(pid_, &status, WNOHANG);
    // SIGTERM?
    kill(pid_, SIGTERM);
    waitpid(pid_, &status, 0); // Wait for termination
    pid_ = -1;
  }
}

bool StdioTransport::send(const std::string &message) {
  if (!running_ || stdin_fd_ == -1)
    return false;

  std::string data = message + "\n";
  ssize_t written = ::write(stdin_fd_, data.c_str(), data.size());
  return written == static_cast<ssize_t>(data.size());
}

void StdioTransport::set_message_handler(MessageHandler handler) {
  std::lock_guard<std::mutex> lock(handler_mutex_);
  handler_ = handler;
}

void StdioTransport::read_loop() {
  std::string buffer;
  char chunk[4096];

  while (running_ && stdout_fd_ != -1) {
    ssize_t n = ::read(stdout_fd_, chunk, sizeof(chunk));
    if (n <= 0)
      break;

    buffer.append(chunk, n);

    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
      std::string line = buffer.substr(0, pos);
      buffer.erase(0, pos + 1);

      // Trim CR
      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      if (!line.empty()) {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        if (handler_)
          handler_(line);
      }
    }
  }
  running_ = false;
}

} // namespace si::mcp
