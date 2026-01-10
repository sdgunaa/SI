#pragma once
#include <string>

namespace si::shell {

class InteractiveShell {
public:
  static InteractiveShell &instance();

  void run();

private:
  InteractiveShell();
  ~InteractiveShell();

  // Non-copyable/moveable
  InteractiveShell(const InteractiveShell &) = delete;
  InteractiveShell &operator=(const InteractiveShell &) = delete;

  bool looks_like_command(const std::string &input);
  void print_block_header(const std::string &cmd);
  void print_block_footer(int exit_code, double duration_ms);
};

} // namespace si::shell
