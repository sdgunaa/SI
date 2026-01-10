#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace si::shell {

/**
 * @brief Result of a command execution
 */
struct ExecutionResult {
  int exit_code = -1;
  std::string stdout_output;
  std::string stderr_output;
  bool success = false;
};

/**
 * @brief Handles shell command execution with output capture
 */
class CommandExecutor {
public:
  CommandExecutor();
  ~CommandExecutor();

  /**
   * @brief Executes a command and waits for completion, capturing all output
   * @param command The bash command to execute
   * @return ExecutionResult containing exit code and captured output
   */
  ExecutionResult execute(const std::string &command);

  /**
   * @brief Executes a command and streams output to callbacks in real-time
   * @param command The bash command to execute
   * @param cwd Working directory
   * @param shell Shell to use (default "/bin/bash")
   * @param on_stdout Callback for stdout chunks
   * @param on_stderr Callback for stderr chunks
   * @return The exit code of the process
   */
  int execute_stream(const std::string &command, const std::string &cwd,
                     const std::string &shell,
                     std::function<void(const std::string &)> on_stdout,
                     std::function<void(const std::string &)> on_stderr,
                     int cols = 80, int rows = 24);

  /**
   * @brief Execute a command and stream output into a Block via BlockManager
   * @param block_id The block to write output to
   * @param command The command to execute
   * @param cwd Working directory
   * @param shell Shell to use
   * @param cols Terminal columns
   * @param rows Terminal rows
   * @return Exit code
   */
  int execute_to_block(const std::string &block_id, const std::string &command,
                       const std::string &cwd = ".",
                       const std::string &shell = "/bin/bash", int cols = 80,
                       int rows = 24);

  /**
   * @brief Simple execution that prints to terminal (like system())
   */
  int run(const std::string &command);
};

} // namespace si::shell
