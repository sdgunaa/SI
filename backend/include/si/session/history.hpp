#pragma once
#include <ctime>
#include <optional>
#include <string>
#include <vector>

namespace si::session {

struct HistoryEntry {
  int id = 0;
  std::string command;
  std::string working_dir;
  int exit_code = 0;
  std::time_t timestamp = 0;
  std::string session_id;
};

class HistoryManager {
public:
  static HistoryManager &instance();

  bool initialize(const std::string &db_path = "");
  void shutdown();

  void add(const std::string &command, const std::string &cwd, int exit_code);
  std::vector<HistoryEntry> search(const std::string &query, int limit = 50);
  std::vector<HistoryEntry> recent(int limit = 50);
  void clear();

private:
  HistoryManager() = default;
  ~HistoryManager();
  HistoryManager(const HistoryManager &) = delete;

  struct Impl;
  Impl *impl_ = nullptr;
  std::string session_id_;
};

} // namespace si::session
