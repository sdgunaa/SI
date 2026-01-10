#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <si/nlohmann/json.hpp>
#include <string>

namespace si::rpc {

using RpcHandler = std::function<nlohmann::json(const nlohmann::json &params)>;

class RpcServer {
public:
  static RpcServer &instance();

  // Register a method handler
  void register_method(const std::string &method_name, RpcHandler handler);

  // Process a single JSON-RPC request string, return response string
  std::string handle_request(const std::string &request_str);

  // Broadcast a notification to all connected clients
  void broadcast(const std::string &method, const nlohmann::json &params);

  // Start the server on a Unix Domain Socket
  bool start(const std::string &socket_path);
  void stop();

private:
  RpcServer() = default;

  void accept_loop();
  void remove_client(int fd);

  std::map<std::string, RpcHandler> methods_;
  std::vector<int> clients_;
  std::mutex mutex_;

  int server_fd_ = -1;
  bool running_ = false;
};

} // namespace si::rpc
