#include "si/mcp/client.hpp"
#include "si/foundation/logging.hpp"
#include <iostream>

namespace si::mcp {

Client::Client(std::unique_ptr<Transport> transport)
    : transport_(std::move(transport)) {
  transport_->set_message_handler(
      [this](const std::string &msg) { this->handle_message(msg); });
}

Client::~Client() {
  if (transport_)
    transport_->close();
}

bool Client::initialize() {
  if (!transport_->start()) {
    SI_LOG_ERROR("Failed to start transport");
    return false;
  }

  nlohmann::json params = {
      {"protocolVersion", "0.1.0"},
      {"capabilities", {}},
      {"clientInfo", {{"name", "SI"}, {"version", "0.1.0"}}}};

  try {
    [[maybe_unused]] auto res = call("initialize", params);

    // Send initialized notification
    nlohmann::json notif = {{"jsonrpc", "2.0"},
                            {"method", "notifications/initialized"},
                            {"params", nlohmann::json::object()}};
    transport_->send(notif.dump());

    initialized_ = true;
    return true;
  } catch (const std::exception &e) {
    SI_LOG_ERROR("MCP Handshake failed: {}", e.what());
    return false;
  }
}

std::vector<Tool> Client::list_tools() {
  if (!initialized_)
    return {};

  try {
    auto res = call("tools/list", nlohmann::json::object());

    std::vector<Tool> tools;
    if (res.contains("tools") && res["tools"].is_array()) {
      for (const auto &t : res["tools"]) {
        tools.push_back(t.get<Tool>());
      }
    }
    return tools;
  } catch (const std::exception &e) {
    SI_LOG_ERROR("Failed to list tools: {}", e.what());
    return {};
  }
}

ToolResult Client::call_tool(const std::string &name,
                             const nlohmann::json &arguments) {
  ToolResult result;
  if (!initialized_) {
    result.is_error = true;
    return result;
  }

  nlohmann::json params = {{"name", name}, {"arguments", arguments}};

  try {
    auto res_json = call("tools/call", params);

    if (res_json.contains("content")) {
      // According to spec, content is list of content items
      if (res_json["content"].is_array()) {
        for (const auto &item : res_json["content"]) {
          result.content.push_back(item);
        }
      }
    }
    if (res_json.contains("isError")) {
      result.is_error = res_json["isError"].get<bool>();
    }

  } catch (const std::exception &e) {
    result.is_error = true;
    nlohmann::json err_content = {{"type", "text"}, {"text", e.what()}};
    result.content.push_back(err_content);
  }
  return result;
}

nlohmann::json Client::call(const std::string &method,
                            const nlohmann::json &params) {
  int id;
  std::future<nlohmann::json> future;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    id = next_id_++;
    auto pr = std::make_unique<PendingRequest>();
    future = pr->promise.get_future();
    pending_requests_[id] = std::move(pr);
  }

  nlohmann::json req = {
      {"jsonrpc", "2.0"}, {"method", method}, {"params", params}, {"id", id}};

  if (!transport_->send(req.dump())) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_requests_.erase(id);
    throw std::runtime_error("Failed to send request");
  }

  // Blocking wait (with timeout ideally, but simple for now)
  if (future.wait_for(std::chrono::seconds(10)) ==
      std::future_status::timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_requests_.erase(id);
    throw std::runtime_error("Request timed out");
  }

  auto response = future.get();

  if (response.contains("error")) {
    std::string msg = "Unknown error";
    if (response["error"].contains("message")) {
      msg = response["error"]["message"].get<std::string>();
    }
    throw std::runtime_error(msg);
  }

  if (response.contains("result")) {
    return response["result"];
  }

  return nlohmann::json::object(); // fallback
}

void Client::handle_message(const std::string &msg) {
  try {
    auto j = nlohmann::json::parse(msg);

    if (j.contains("id") && j["id"].is_number()) {
      int id = j["id"].get<int>();
      std::lock_guard<std::mutex> lock(mutex_);
      if (pending_requests_.count(id)) {
        pending_requests_[id]->promise.set_value(j);
        pending_requests_.erase(id);
      }
    }
    // Notifications (no id) handling could go here

  } catch (const std::exception &e) {
    SI_LOG_ERROR("Failed to parse incoming message: {} [{}]", e.what(), msg);
  }
}

} // namespace si::mcp
