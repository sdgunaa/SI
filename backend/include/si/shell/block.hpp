#pragma once

#include <chrono>
#include <map>
#include <si/nlohmann/json.hpp>
#include <string>
#include <vector>

namespace si::shell {

enum class BlockState { RUNNING, COMPLETED, FAILED, CANCELLED };

struct OutputChunk {
  std::string data;
  std::string type;  // "stdout", "stderr", "html", "json"
  int64_t timestamp; // ms since epoch
};

struct Block {
  std::string id; // UUID
  std::string session_id;
  std::string command;
  std::string cwd;
  std::map<std::string, std::string> env;

  int64_t start_time = 0;
  int64_t end_time = 0;
  int exit_code = 0;
  BlockState state = BlockState::RUNNING;

  std::vector<OutputChunk> output_chunks;
  nlohmann::json metadata; // Tags, AI analysis, etc.

  // Helper to add output
  void add_output(const std::string &data, const std::string &type = "stdout") {
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    output_chunks.push_back({data, type, ts});
  }
};

// JSON Serialization
inline void to_json(nlohmann::json &j, const OutputChunk &c) {
  j = nlohmann::json{{"data", c.data}, {"type", c.type}, {"ts", c.timestamp}};
}
inline void from_json(const nlohmann::json &j, OutputChunk &c) {
  j.at("data").get_to(c.data);
  j.at("type").get_to(c.type);
  j.at("ts").get_to(c.timestamp);
}

inline void to_json(nlohmann::json &j, const Block &b) {
  j = nlohmann::json{{"id", b.id},
                     {"session_id", b.session_id},
                     {"command", b.command},
                     {"cwd", b.cwd},
                     {"env", b.env},
                     {"start_time", b.start_time},
                     {"end_time", b.end_time},
                     {"exit_code", b.exit_code},
                     {"state", static_cast<int>(b.state)},
                     {"output_chunks", b.output_chunks},
                     {"metadata", b.metadata}};
}
inline void from_json(const nlohmann::json &j, Block &b) {
  j.at("id").get_to(b.id);
  j.at("session_id").get_to(b.session_id);
  j.at("command").get_to(b.command);
  j.at("cwd").get_to(b.cwd);
  if (j.contains("env"))
    j.at("env").get_to(b.env);
  j.at("start_time").get_to(b.start_time);
  j.at("end_time").get_to(b.end_time);
  j.at("exit_code").get_to(b.exit_code);
  b.state = static_cast<BlockState>(j.at("state").get<int>());
  if (j.contains("output_chunks"))
    j.at("output_chunks").get_to(b.output_chunks);
  if (j.contains("metadata"))
    j.at("metadata").get_to(b.metadata);
}

} // namespace si::shell
