#pragma once

#include "si/ai/context_builder.hpp"
#include "si/ai/gateway.hpp"
#include "si/rpc/server.hpp"
#include "si/settings/settings_manager.hpp"
#include "si/shell/block_manager.hpp"
#include "si/shell/executor.hpp"
#include "si/shell/workflow_engine.hpp"
#include <filesystem>
#include <fstream>

namespace si::rpc {

/**
 * Registers all Core API methods with the RPC Server
 */
inline void register_api_bindings() {
  auto &rpc = RpcServer::instance();
  auto &blocks = si::shell::BlockManager::instance();
  auto &workflows = si::shell::WorkflowEngine::instance();
  static si::shell::CommandExecutor executor;

  // Set up notifications
  blocks.set_update_callback([](const std::string &block_id,
                                const si::shell::OutputChunk &chunk) {
    RpcServer::instance().broadcast(
        "block.output",
        {{"block_id", block_id}, {"data", chunk.data}, {"type", chunk.type}});
  });

  blocks.set_complete_callback([](const std::string &block_id,
                                  const std::string &session_id,
                                  int exit_code) {
    RpcServer::instance().broadcast(
        "block.complete", {
                              {"block_id", block_id},
                              {"session_id", session_id},
                              {"exit_code", exit_code},
                              {"duration_ms", 0} // TODO: Track duration
                          });
  });

  // Block API
  rpc.register_method("block.create", [&](const nlohmann::json &p) {
    std::string id = blocks.create_block(p.value("session_id", "default"),
                                         p.at("command").get<std::string>(),
                                         p.value("cwd", "."));
    return nlohmann::json{{"block_id", id}};
  });

  rpc.register_method("block.execute", [&](const nlohmann::json &p) {
    std::string session_id = p.value("session_id", "default");
    std::string command = p.at("command").get<std::string>();

    auto &ctx = blocks.get_session_context(session_id);
    std::string cwd = p.value("cwd", ctx.cwd);
    std::string shell = ctx.shell;

    // Track the final CWD - will be updated if this is a cd command
    std::string final_cwd = cwd;

    // Detect 'cd' command to update session CWD
    if (command == "cd" || command.substr(0, 3) == "cd ") {
      SI_LOG_INFO("Detected cd command: '{}', current CWD: {}", command, cwd);
      std::string target_path;
      if (command == "cd" || command.length() == 3) {
        const char *home = getenv("HOME");
        target_path = home ? home : "/";
      } else {
        std::string raw_path = command.substr(3);
        if (!raw_path.empty() && raw_path.back() == '\n') {
          raw_path.pop_back();
        }
        target_path = raw_path;
      }

      std::filesystem::path new_path;
      if (std::filesystem::path(target_path).is_absolute()) {
        new_path = std::filesystem::path(target_path);
      } else {
        new_path = std::filesystem::path(cwd) / target_path;
      }

      try {
        std::filesystem::path resolved =
            std::filesystem::weakly_canonical(new_path);
        if (std::filesystem::exists(resolved) &&
            std::filesystem::is_directory(resolved)) {
          SI_LOG_INFO("Updating session {} CWD: {} -> {}", session_id, cwd,
                      resolved.string());
          blocks.set_session_cwd(session_id, resolved.string());
          // CRITICAL: Store the new CWD for the response
          final_cwd = resolved.string();
        } else {
          SI_LOG_WARN("cd target does not exist or is not a directory: {}",
                      resolved.string());
        }
      } catch (const std::filesystem::filesystem_error &e) {
        SI_LOG_WARN("cd path resolution failed: {}", e.what());
      }
    }

    // Create block and execute (uses OLD cwd for the shell process)
    std::string block_id = blocks.create_block(session_id, command, cwd);

    int cols = p.value("cols", 80);
    int rows = p.value("rows", 24);

    std::thread([block_id, command, cwd, shell, cols, rows]() {
      si::shell::CommandExecutor exec;
      exec.execute_to_block(block_id, command, cwd, shell, cols, rows);
    }).detach();

    // Return with the tracked final_cwd (which is updated after cd commands)
    // SI_LOG_INFO("Returning session_config: cwd={}, shell={}", final_cwd,
    // shell); // Removed spam
    return nlohmann::json{
        {"block_id", block_id},
        {"session_config", {{"cwd", final_cwd}, {"shell", shell}}}};
  });

  rpc.register_method("block.get", [&](const nlohmann::json &p) {
    auto block = blocks.get_block(p.at("block_id").get<std::string>());
    if (block)
      return nlohmann::json(*block);
    throw std::runtime_error("Block not found");
  });

  // Session API
  rpc.register_method("session.create", [&](const nlohmann::json &p) {
    std::string name = p.value("name", "New Session");
    std::string id = blocks.create_session(name);
    return nlohmann::json{{"session_id", id}, {"name", name}};
  });

  rpc.register_method("session.list", [&](const nlohmann::json &p) {
    auto sessions = blocks.list_sessions();
    nlohmann::json list = nlohmann::json::array();
    for (const auto &s : sessions) {
      list.push_back({{"id", s.first}, {"name", s.second}});
    }
    return nlohmann::json(list);
  });

  rpc.register_method("session.delete", [&](const nlohmann::json &p) {
    std::string session_id = p.at("session_id").get<std::string>();
    blocks.delete_session(session_id);
    return nlohmann::json{{"success", true}};
  });

  rpc.register_method("session.rename", [&](const nlohmann::json &p) {
    std::string session_id = p.at("session_id").get<std::string>();
    std::string name = p.at("name").get<std::string>();
    blocks.rename_session(session_id, name);
    return nlohmann::json{{"success", true}};
  });

  rpc.register_method("block.list", [&](const nlohmann::json &p) {
    auto list = blocks.list_blocks(p.value("session_id", "default"));
    return nlohmann::json(list);
  });

  // Session Config API
  rpc.register_method("session.get_config", [&](const nlohmann::json &p) {
    std::string session_id = p.value("session_id", "default");
    try {
      // CRITICAL FIX: Use thread-safe copy helper to avoid race conditions
      auto [cwd_copy, shell_copy] = blocks.get_session_config_copy(session_id);
      // SI_LOG_INFO("[session.get_config] returning cwd={}, shell={}",
      // cwd_copy, shell_copy); // Removed spam
      return nlohmann::json{{"cwd", cwd_copy}, {"shell", shell_copy}};
    } catch (...) {
      // Fallback if session deleted or invalid
      return nlohmann::json{{"cwd", "."}, {"shell", "/bin/bash"}};
    }
  });

  rpc.register_method("session.set_config", [&](const nlohmann::json &p) {
    std::string session_id = p.value("session_id", "default");
    if (p.contains("cwd")) {
      blocks.set_session_cwd(session_id, p["cwd"].get<std::string>());
    }
    if (p.contains("shell")) {
      blocks.set_session_shell(session_id, p["shell"].get<std::string>());
    }
    return nlohmann::json{{"success", true}};
  });

  // FS API
  rpc.register_method("fs.list", [](const nlohmann::json &p) {
    std::string path_str = p.at("path").get<std::string>();
    std::string path_resolved;

    // Handle ~
    if (path_str == "~" || path_str.substr(0, 2) == "~/") {
      const char *home = getenv("HOME");
      if (home) {
        path_resolved = std::string(home) + path_str.substr(1);
      } else {
        path_resolved = path_str;
      }
    } else {
      path_resolved = path_str;
    }

    std::filesystem::path path(path_resolved);

    // Attempt to make absolute if relative (relative to server CWD, which is
    // usually project root or build dir) Note: Ideally relative paths should be
    // relative to session CWD, but fs.list is session-agnostic currently.
    // Frontend passes session CWD usually.

    if (!std::filesystem::exists(path)) {
      throw std::runtime_error("Path does not exist: " + path.string());
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
      nlohmann::json item;
      item["name"] = entry.path().filename().string();
      item["is_directory"] = entry.is_directory();
      item["size"] = entry.is_regular_file() ? entry.file_size() : 0;
      item["mtime"] = std::chrono::duration_cast<std::chrono::seconds>(
                          entry.last_write_time().time_since_epoch())
                          .count();
      result.push_back(item);
    }
    return result;
  });

  rpc.register_method("fs.read", [](const nlohmann::json &p) {
    std::string path_str = p.at("path").get<std::string>();
    std::filesystem::path path(path_str);

    if (!std::filesystem::exists(path)) {
      throw std::runtime_error("File does not exist: " + path_str);
    }
    if (!std::filesystem::is_regular_file(path)) {
      throw std::runtime_error("Not a regular file: " + path_str);
    }
    // Limit file size to avoid memory issues (e.g. 10MB)
    if (std::filesystem::file_size(path) > 10 * 1024 * 1024) {
      throw std::runtime_error("File too large (>10MB): " + path_str);
    }

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
      throw std::runtime_error("Failed to open file: " + path_str);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
  });

  rpc.register_method("fs.write", [](const nlohmann::json &p) {
    std::string path_str = p.at("path").get<std::string>();
    std::string content = p.value("content", "");
    std::filesystem::path path(path_str);

    // Basic security check: prevent writing outside of home/projects generally?
    // For now, assume user trusts the app running as them.

    std::ofstream file(path,
                       std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) {
      throw std::runtime_error("Failed to open file for writing: " + path_str);
    }

    file << content;
    if (file.fail()) {
      throw std::runtime_error("Failed to write to file: " + path_str);
    }

    return nlohmann::json{{"success", true}};
  });

  // Workflow API
  rpc.register_method("workflow.save", [&](const nlohmann::json &p) {
    si::shell::Workflow w = p.get<si::shell::Workflow>();
    std::string id = workflows.save_workflow(w);
    return nlohmann::json{{"workflow_id", id}};
  });

  rpc.register_method("workflow.get", [&](const nlohmann::json &p) {
    auto w = workflows.get_workflow(p.at("workflow_id").get<std::string>());
    if (w)
      return nlohmann::json(*w);
    throw std::runtime_error("Workflow not found");
  });

  rpc.register_method("workflow.list", [&](const nlohmann::json &p) {
    auto list = workflows.list_workflows(p.value("tag", ""));
    return nlohmann::json(list);
  });

  rpc.register_method("workflow.render", [&](const nlohmann::json &p) {
    std::string cmd = workflows.render_command(
        p.at("workflow_id").get<std::string>(),
        p.value("params", std::map<std::string, std::string>{}));
    return nlohmann::json{{"command", cmd}};
  });

  // AI API
  rpc.register_method("ai.get_context", [](const nlohmann::json &p) {
    auto &ctx_builder = si::ai::ContextBuilder::instance();
    if (p.contains("cwd"))
      ctx_builder.set_cwd(p["cwd"].get<std::string>());
    if (p.contains("session_id"))
      ctx_builder.set_session_id(p["session_id"].get<std::string>());
    return ctx_builder.build_context();
  });

  rpc.register_method("ai.generate_command", [](const nlohmann::json &p) {
    auto &gateway = si::ai::AIGateway::instance();
    auto &ctx_builder = si::ai::ContextBuilder::instance();

    std::string user_prompt = p.at("prompt").get<std::string>();
    nlohmann::json context = ctx_builder.build_context();

    si::ai::CompletionRequest req;
    // Combine system prompt and user prompt into single prompt
    req.prompt = ctx_builder.get_command_generation_prompt() +
                 "\n\nContext: " + context.dump() +
                 "\n\nRequest: " + user_prompt;
    req.max_tokens = 256;
    req.temperature = 0.3f;

    auto resp = gateway.complete(req);

    return nlohmann::json{{"command", resp.content}, {"success", resp.success}};
  });

  rpc.register_method("ai.analyze_error", [](const nlohmann::json &p) {
    auto &gateway = si::ai::AIGateway::instance();
    auto &ctx_builder = si::ai::ContextBuilder::instance();
    auto &blocks = si::shell::BlockManager::instance();

    std::string block_id = p.at("block_id").get<std::string>();
    auto block_opt = blocks.get_block(block_id);
    if (!block_opt)
      throw std::runtime_error("Block not found");

    auto &block = *block_opt;
    std::string output;
    for (const auto &chunk : block.output_chunks) {
      output += chunk.data;
    }

    si::ai::CompletionRequest req;
    req.prompt = ctx_builder.get_error_analysis_prompt() +
                 "\n\nCommand: " + block.command +
                 "\nExit Code: " + std::to_string(block.exit_code) +
                 "\nOutput:\n" + output;
    req.max_tokens = 512;

    auto resp = gateway.complete(req);

    return nlohmann::json{{"analysis", resp.content},
                          {"success", resp.success}};
  });

  // Settings API
  auto &settings = si::settings::SettingsManager::instance();

  rpc.register_method("settings.get", [&](const nlohmann::json &p) {
    std::string category = p.at("category").get<std::string>();
    return settings.get_category(category);
  });

  rpc.register_method("settings.set", [&](const nlohmann::json &p) {
    std::string category = p.at("category").get<std::string>();
    settings.set_category(category, p.at("data"));
    return nlohmann::json{{"success", true}};
  });

  rpc.register_method("settings.reset", [&](const nlohmann::json &p) {
    std::string category = p.at("category").get<std::string>();
    settings.reset_category(category);
    return nlohmann::json{{"success", true}};
  });
}

} // namespace si::rpc
