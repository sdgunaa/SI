#pragma once

#include "si/shell/block.hpp"
#include <functional>
#include <map>
#include <mutex>
#include <si/nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace si::shell {

class BlockManager {
public:
  static BlockManager &instance();

  // Session context management
  struct SessionContext {
    std::string cwd = ".";
    std::string shell = "/bin/bash";
    std::string name = "Default";
  };

  SessionContext &get_session_context(const std::string &session_id);

  // Creates a new session with a name and returns its ID
  std::string create_session(const std::string &name);

  // Lists all sessions (id, name)
  std::vector<std::pair<std::string, std::string>> list_sessions();

  void delete_session(const std::string &session_id);
  void rename_session(const std::string &session_id, const std::string &name);

  void set_session_cwd(const std::string &session_id, const std::string &cwd);
  void set_session_shell(const std::string &session_id,
                         const std::string &shell);

  // Thread-safe: Returns a COPY of cwd and shell while holding the lock
  std::pair<std::string, std::string>
  get_session_config_copy(const std::string &session_id);

  // Create a new running block
  std::string create_block(const std::string &session_id,
                           const std::string &command,
                           const std::string &cwd = "");

  // Update block output
  void append_output(const std::string &block_id, const std::string &data,
                     const std::string &type = "stdout");

  // Complete block
  void complete_block(const std::string &block_id, int exit_code);

  // Get a specific block
  std::optional<Block> get_block(const std::string &block_id);

  // List blocks for a session
  std::vector<Block> list_blocks(const std::string &session_id);

  // Callbacks for API events
  using BlockUpdateCallback = std::function<void(const std::string &block_id,
                                                 const OutputChunk &chunk)>;
  using BlockCompleteCallback =
      std::function<void(const std::string &block_id,
                         const std::string &session_id, int exit_code)>;

  void set_update_callback(BlockUpdateCallback cb);
  void set_complete_callback(BlockCompleteCallback cb);

  // Persistence
  void save_sessions();
  void load_sessions();

private:
  BlockManager();

  // Generates UUID
  std::string generate_uuid();

  std::map<std::string, Block> blocks_;            // block_id -> Block
  std::map<std::string, SessionContext> sessions_; // session_id -> Context
  std::mutex mutex_;

  BlockUpdateCallback update_cb_;
  BlockCompleteCallback complete_cb_;

  // Internal save without lock
  void save_sessions_internal();
};

} // namespace si::shell
