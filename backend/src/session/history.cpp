#include "si/session/history.hpp"
#include "si/foundation/config.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include <algorithm>
#include <fstream>
#include <random>
#include <sqlite3.h>
#include <sstream>

namespace si::session {

struct HistoryManager::Impl {
  std::string file_path;
  std::vector<HistoryEntry> entries;
  int next_id = 1;

  void load() {
    std::ifstream file(file_path);
    if (!file)
      return;

    std::string line;
    while (std::getline(file, line)) {
      if (line.empty())
        continue;

      HistoryEntry e;
      std::istringstream iss(line);
      iss >> e.id >> e.exit_code >> e.timestamp;
      iss.ignore();
      std::getline(iss, e.working_dir, '\t');
      std::getline(iss, e.command);

      entries.push_back(e);
      if (e.id >= next_id)
        next_id = e.id + 1;
    }
  }

  void save() {
    std::ofstream file(file_path, std::ios::trunc);
    for (const auto &e : entries) {
      file << e.id << " " << e.exit_code << " " << e.timestamp << "\t"
           << e.working_dir << "\t" << e.command << "\n";
    }
  }
};

HistoryManager &HistoryManager::instance() {
  static HistoryManager inst;
  return inst;
}

HistoryManager::~HistoryManager() { shutdown(); }

bool HistoryManager::initialize(const std::string &db_path) {
  if (impl_)
    return true;

  impl_ = new Impl();

  if (db_path.empty()) {
    auto data_dir = si::foundation::Platform::get_data_dir();
    impl_->file_path = (data_dir / "history.txt").string();
  } else {
    impl_->file_path = db_path;
  }

  // Generate session ID
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  const char *hex = "0123456789abcdef";
  session_id_.clear();
  for (int i = 0; i < 8; i++)
    session_id_ += hex[dis(gen)];

  impl_->load();
  SI_LOG_INFO("History loaded: {} entries", impl_->entries.size());
  return true;
}

void HistoryManager::shutdown() {
  if (impl_) {
    impl_->save();
    delete impl_;
    impl_ = nullptr;
  }
}

void HistoryManager::add(const std::string &command, const std::string &cwd,
                         int exit_code) {
  if (!impl_)
    return;

  HistoryEntry e;
  e.id = impl_->next_id++;
  e.command = command;
  e.working_dir = cwd;
  e.exit_code = exit_code;
  e.timestamp = std::time(nullptr);
  e.session_id = session_id_;

  impl_->entries.push_back(e);

  // Auto-save every 10 entries
  if (impl_->entries.size() % 10 == 0) {
    impl_->save();
  }
}

std::vector<HistoryEntry> HistoryManager::search(const std::string &query,
                                                 int limit) {
  if (!impl_)
    return {};

  std::vector<HistoryEntry> results;
  for (auto it = impl_->entries.rbegin();
       it != impl_->entries.rend() && (int)results.size() < limit; ++it) {
    if (it->command.find(query) != std::string::npos) {
      results.push_back(*it);
    }
  }
  return results;
}

std::vector<HistoryEntry> HistoryManager::recent(int limit) {
  if (!impl_)
    return {};

  std::vector<HistoryEntry> results;
  int start = std::max(0, (int)impl_->entries.size() - limit);
  for (size_t i = start; i < impl_->entries.size(); i++) {
    results.push_back(impl_->entries[i]);
  }
  std::reverse(results.begin(), results.end());
  return results;
}

void HistoryManager::clear() {
  if (!impl_)
    return;
  impl_->entries.clear();
  impl_->save();
}

} // namespace si::session
