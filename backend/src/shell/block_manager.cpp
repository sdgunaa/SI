#include "si/shell/block_manager.hpp"
#include "si/foundation/logging.hpp"
#include "si/foundation/platform.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <si/nlohmann/json.hpp>
#include <random>
#include <set>
#include <sstream>

namespace si::shell {

namespace {
// Get path to sessions.json in user data directory
std::filesystem::path get_sessions_file_path() {
  auto data_dir = si::foundation::Platform::get_data_dir();
  // Ensure directory exists
  if (!std::filesystem::exists(data_dir)) {
    std::filesystem::create_directories(data_dir);
  }
  return data_dir / "sessions.json";
}
} // anonymous namespace

BlockManager &BlockManager::instance() {
  static BlockManager inst;
  // Load sessions on first access if not already loaded (though constructor
  // handles it)
  return inst;
}

BlockManager::BlockManager() { load_sessions(); }

// ... uuid gen ...

std::string BlockManager::generate_uuid() {
  // Simple UUID v4 generator
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  static std::uniform_int_distribution<> dis2(8, 11);

  std::stringstream ss;
  int i;
  ss << std::hex;
  for (i = 0; i < 8; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (i = 0; i < 4; i++) {
    ss << dis(gen);
  }
  ss << "-4";
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  ss << dis2(gen);
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (i = 0; i < 12; i++) {
    ss << dis(gen);
  }
  return ss.str();
}

BlockManager::SessionContext &
BlockManager::get_session_context(const std::string &session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return sessions_[session_id];
}

void BlockManager::set_session_cwd(const std::string &session_id,
                                   const std::string &cwd) {
  std::lock_guard<std::mutex> lock(mutex_);
  // SI_LOG_INFO("[set_session_cwd] Updating CWD: {}", cwd);
  sessions_[session_id].cwd = cwd;
}

std::pair<std::string, std::string>
BlockManager::get_session_config_copy(const std::string &session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto &ctx = sessions_[session_id];
  return {ctx.cwd, ctx.shell};
}

void BlockManager::set_session_shell(const std::string &session_id,
                                     const std::string &shell) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_[session_id].shell = shell;
}

std::string BlockManager::create_session(const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string id = generate_uuid();
  sessions_[id].name = name;
  try {
    sessions_[id].cwd = std::filesystem::current_path().string();
  } catch (...) {
    sessions_[id].cwd = "/";
  }
  sessions_[id].shell = "/bin/bash"; // Default shell
  SI_LOG_INFO("Created Session: {} [{}]", id, name);
  save_sessions_internal();
  return id;
}

std::vector<std::pair<std::string, std::string>> BlockManager::list_sessions() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Identify which sessions have blocks
  std::set<std::string> session_with_blocks;
  for (const auto &[id, b] : blocks_) {
    session_with_blocks.insert(b.session_id);
  }

  std::vector<std::pair<std::string, std::string>> result;
  for (const auto &[id, ctx] : sessions_) {
    bool has_blocks = session_with_blocks.count(id) > 0;
    bool is_renamed = ctx.name != "New Session";

    if (has_blocks || is_renamed) {
      result.push_back({id, ctx.name});
    }
  }
  return result;
}

void BlockManager::delete_session(const std::string &session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_.erase(session_id);
  // Optional: delete blocks associated with session?
  // blocks_.erase_if... but for now keeping history might be safer or separate
  // cleanup
  save_sessions_internal();
}

void BlockManager::rename_session(const std::string &session_id,
                                  const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (sessions_.count(session_id)) {
    sessions_[session_id].name = name;
    save_sessions_internal();
  }
}

std::string BlockManager::create_block(const std::string &session_id,
                                       const std::string &command,
                                       const std::string &cwd) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string id = generate_uuid();
  Block b;
  b.id = id;
  b.session_id = session_id;
  b.command = command;

  // Use session's CWD if none provided
  if (cwd.empty()) {
    b.cwd = sessions_[session_id].cwd;
  } else {
    b.cwd = cwd;
    // CRITICAL FIX: Do NOT overwrite session CWD with block CWD.
    // The block runs in the *current* (potentially old) CWD.
    // Session CWD updates are handled explicitly by 'cd' command logic.
    // sessions_[session_id].cwd = cwd; // REMOVED
  }

  b.state = BlockState::RUNNING;

  auto now = std::chrono::system_clock::now();
  b.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now.time_since_epoch())
                     .count();

  blocks_[id] = b;

  SI_LOG_INFO("Created Block: {} [{}] in {}", id, command, b.cwd);
  save_sessions_internal();
  return id;
}

void BlockManager::append_output(const std::string &block_id,
                                 const std::string &data,
                                 const std::string &type) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string safe_data = data;
  try {
    // Validate UTF-8 by attempting to create AND serialize a JSON string.
    nlohmann::json j = data;
    j.dump();
  } catch (const nlohmann::json::exception &) {
    // If validation fails (e.g., binary output from 'cat pdf'), sanitize it.
    std::string filtered;
    filtered.reserve(data.size());
    for (unsigned char c : data) {
      if (c < 128) {
        filtered += c;
      } else {
        // Replace invalid/non-ASCII byte with replacement character
        filtered += "?";
      }
    }
    safe_data = filtered;
  }

  if (blocks_.count(block_id)) {
    blocks_[block_id].add_output(safe_data, type);

    // Notify
    if (update_cb_) {
      try {
        // Need to construct chunk with timestamp we just made.
        // Ideally add_output returns the chunk or we make it here.
        auto &chunks = blocks_[block_id].output_chunks;
        if (!chunks.empty())
          update_cb_(block_id, chunks.back());
      } catch (const std::exception &e) {
        SI_LOG_ERROR("Failed to dispatch update callback: {}", e.what());
      }
    }
  }
}

void BlockManager::complete_block(const std::string &block_id, int exit_code) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (blocks_.count(block_id)) {
    auto &b = blocks_[block_id];
    b.exit_code = exit_code;
    b.state = (exit_code == 0) ? BlockState::COMPLETED : BlockState::FAILED;

    auto now = std::chrono::system_clock::now();
    b.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now.time_since_epoch())
                     .count();

    SI_LOG_INFO("Block Complete: {} [Code: {}]", block_id, exit_code);
    save_sessions_internal();

    if (complete_cb_) {
      complete_cb_(block_id, b.session_id, exit_code);
    }
  }
}

std::optional<Block> BlockManager::get_block(const std::string &block_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (blocks_.count(block_id)) {
    return blocks_[block_id];
  }
  return std::nullopt;
}

std::vector<Block> BlockManager::list_blocks(const std::string &session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Block> result;
  for (const auto &[id, block] : blocks_) {
    if (block.session_id == session_id) {
      result.push_back(block);
    }
  }
  // Sorting by start_time? Map is vaguely ordered by ID not time.
  // Should sort.
  std::sort(result.begin(), result.end(), [](const Block &a, const Block &b) {
    return a.start_time < b.start_time;
  });
  return result;
}

void BlockManager::save_sessions() {
  std::lock_guard<std::mutex> lock(mutex_);
  save_sessions_internal();
}

void BlockManager::save_sessions_internal() {
  nlohmann::json j;
  j["sessions"] = nlohmann::json::array();
  j["blocks"] = nlohmann::json::array();

  // 1. Identify which sessions have blocks
  std::set<std::string> session_with_blocks;
  for (const auto &[id, b] : blocks_) {
    session_with_blocks.insert(b.session_id);
  }

  // 2. Only save sessions that are non-empty or have been renamed (Spec
  // requirement)
  std::set<std::string> saved_session_ids;
  for (const auto &[id, ctx] : sessions_) {
    bool has_blocks = session_with_blocks.count(id) > 0;
    bool is_renamed = ctx.name != "New Session";

    if (has_blocks || is_renamed) {
      nlohmann::json s;
      s["id"] = id;
      s["name"] = ctx.name;
      s["cwd"] = ctx.cwd;
      s["shell"] = ctx.shell;
      j["sessions"].push_back(s);
      saved_session_ids.insert(id);
    }
  }

  // 3. Only save blocks belonging to saved sessions
  for (const auto &[id, b] : blocks_) {
    if (saved_session_ids.count(b.session_id) > 0) {
      nlohmann::json blk;
      blk = b; // Uses to_json from block.hpp
      j["blocks"].push_back(blk);
    }
  }

  auto sessions_path = get_sessions_file_path();
  std::ofstream o(sessions_path);
  o << j.dump(2) << std::endl;
}

void BlockManager::load_sessions() {
  std::lock_guard<std::mutex> lock(mutex_);
  auto sessions_path = get_sessions_file_path();
  if (!std::filesystem::exists(sessions_path))
    return;

  try {
    std::ifstream i(sessions_path);
    if (!i.is_open())
      return;

    nlohmann::json j;
    i >> j;

    sessions_.clear();
    if (j.contains("sessions")) {
      for (const auto &s : j["sessions"]) {
        SessionContext ctx;
        std::string id = s.value("id", "");
        if (id.empty())
          continue;
        ctx.name = s.value("name", "Default");
        ctx.cwd = s.value("cwd", ".");
        ctx.shell = s.value("shell", "/bin/bash");
        sessions_[id] = ctx;
      }
    }

    blocks_.clear();
    if (j.contains("blocks")) {
      for (const auto &blk : j["blocks"]) {
        Block b = blk.get<Block>(); // Uses from_json
        blocks_[b.id] = b;
      }
    }
    SI_LOG_INFO("Loaded {} sessions and {} blocks", sessions_.size(),
                blocks_.size());
  } catch (const std::exception &e) {
    SI_LOG_ERROR("Failed to load sessions: {}", e.what());
  }
}

void BlockManager::set_update_callback(BlockUpdateCallback cb) {
  std::lock_guard<std::mutex> lock(mutex_);
  update_cb_ = cb;
}

void BlockManager::set_complete_callback(BlockCompleteCallback cb) {
  std::lock_guard<std::mutex> lock(mutex_);
  complete_cb_ = cb;
}

} // namespace si::shell
