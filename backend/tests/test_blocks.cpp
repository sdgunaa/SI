#include "si/foundation/logging.hpp"
#include "si/shell/block_manager.hpp"
#include <catch2/catch_all.hpp>

using namespace si::shell;

TEST_CASE("Block Manager Basics", "[blocks]") {
  // Init logger
  static bool log_init = false;
  if (!log_init) {
    try {
      si::foundation::Logger::instance().init(
          "test_blocks.log", si::foundation::Logger::Level::Debug,
          si::foundation::Logger::Level::Debug);
    } catch (...) {
    }
    log_init = true;
  }

  auto &bm = BlockManager::instance();
  std::string session_id = "test-session-1";

  SECTION("Create and Get") {
    std::string id = bm.create_block(session_id, "echo hello", "/tmp");
    REQUIRE(!id.empty());

    auto block = bm.get_block(id);
    REQUIRE(block.has_value());
    REQUIRE(block->command == "echo hello");
    REQUIRE(block->state == BlockState::RUNNING);
  }

  SECTION("Output and Updates") {
    std::string id = bm.create_block(session_id, "ls", "/");

    bool callback_fired = false;
    bm.set_update_callback(
        [&](const std::string &bidirectional_id, const OutputChunk &chunk) {
          if (bidirectional_id == id) {
            callback_fired = true;
            REQUIRE(chunk.data == "file1.txt");
          }
        });

    bm.append_output(id, "file1.txt");
    REQUIRE(callback_fired);

    auto block = bm.get_block(id);
    REQUIRE(block->output_chunks.size() == 1);
    REQUIRE(block->output_chunks[0].data == "file1.txt");
  }

  SECTION("Completion") {
    std::string id = bm.create_block(session_id, "exit 0", "/");

    bool completed_fired = false;
    bm.set_complete_callback(
        [&](const std::string &cb_id, const std::string &session_id, int exit) {
          if (cb_id == id) {
            completed_fired = true;
            REQUIRE(exit == 0);
          }
        });

    bm.complete_block(id, 0);
    REQUIRE(completed_fired);

    auto block = bm.get_block(id);
    REQUIRE(block->state == BlockState::COMPLETED);
    REQUIRE(block->end_time > 0);
  }

  SECTION("List Blocks") {
    auto list = bm.list_blocks(session_id);
    REQUIRE(list.size() >= 3); // From previous sections
  }
}
