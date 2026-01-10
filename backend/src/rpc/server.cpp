#include "si/rpc/server.hpp"
#include "si/foundation/logging.hpp"
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace si::rpc {

RpcServer &RpcServer::instance() {
  static RpcServer inst;
  return inst;
}

void RpcServer::register_method(const std::string &method_name,
                                RpcHandler handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  methods_[method_name] = handler;
  SI_LOG_INFO("RPC: Registered method '{}'", method_name);
}

std::string RpcServer::handle_request(const std::string &request_str) {
  nlohmann::json response;

  try {
    auto request = nlohmann::json::parse(request_str);

    // Check JSON-RPC version
    if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
      response = {{"jsonrpc", "2.0"},
                  {"error", {{"code", -32600}, {"message", "Invalid Request"}}},
                  {"id", nullptr}};
      return response.dump();
    }

    std::string method = request["method"].get<std::string>();
    auto id = request.value("id", nlohmann::json(nullptr));
    auto params = request.value("params", nlohmann::json::object());

    std::lock_guard<std::mutex> lock(mutex_);
    if (methods_.count(method)) {
      try {
        auto result = methods_[method](params);
        if (!id.is_null()) {
          response = {{"jsonrpc", "2.0"}, {"result", result}, {"id", id}};
        }
      } catch (const std::exception &e) {
        response = {{"jsonrpc", "2.0"},
                    {"error", {{"code", -32000}, {"message", e.what()}}},
                    {"id", id}};
      }
    } else {
      response = {
          {"jsonrpc", "2.0"},
          {"error", {{"code", -32601}, {"message", "Method not found"}}},
          {"id", id}};
    }

  } catch (const nlohmann::json::parse_error &e) {
    response = {{"jsonrpc", "2.0"},
                {"error", {{"code", -32700}, {"message", "Parse error"}}},
                {"id", nullptr}};
  }

  return response.dump();
}

bool RpcServer::start(const std::string &socket_path) {
  if (running_)
    return true;

  // Remove old socket file if exists
  unlink(socket_path.c_str());

  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    SI_LOG_ERROR("RPC: Failed to create socket");
    return false;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(server_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    SI_LOG_ERROR("RPC: Failed to bind socket to {}", socket_path);
    ::close(server_fd_);
    return false;
  }

  if (listen(server_fd_, 50) < 0) {
    SI_LOG_ERROR("RPC: Failed to listen");
    ::close(server_fd_);
    return false;
  }

  running_ = true;
  SI_LOG_INFO("RPC: Server started on {}", socket_path);

  // Start accept loop in background thread
  std::thread([this]() { this->accept_loop(); }).detach();

  return true;
}

void RpcServer::stop() {
  running_ = false;
  if (server_fd_ >= 0) {
    ::close(server_fd_);
    server_fd_ = -1;
  }
}

void RpcServer::accept_loop() {
  while (running_) {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
      if (running_)
        SI_LOG_WARN("RPC: Accept failed");
      continue;
    }

    SI_LOG_INFO("RPC: New client connection accepted");

    {
      std::lock_guard<std::mutex> lock(mutex_);
      clients_.push_back(client_fd);
    }

    // Handle client in a new thread
    std::thread([this, client_fd]() {
      char buffer[65536];
      std::string accumulated;

      while (true) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
          if (n < 0 && errno == EINTR) {
            continue;
          }
          if (n == 0) {
            SI_LOG_INFO("RPC: Client disconnected cleanly");
          } else {
            SI_LOG_ERROR("RPC: Recv failed: {}", std::strerror(errno));
          }
          break;
        }

        buffer[n] = 0;
        accumulated += buffer;

        // Simple newline-delimited messages
        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
          std::string line = accumulated.substr(0, pos);
          accumulated.erase(0, pos + 1);

          if (!line.empty()) {
            std::string response = handle_request(line);
            response += "\n";
            send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
          }
        }
      }

      remove_client(client_fd);
      ::close(client_fd);
    }).detach();
  }
}

void RpcServer::remove_client(int fd) {
  std::lock_guard<std::mutex> lock(mutex_);
  clients_.erase(std::remove(clients_.begin(), clients_.end(), fd),
                 clients_.end());
}

void RpcServer::broadcast(const std::string &method,
                          const nlohmann::json &params) {
  nlohmann::json msg{
      {"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
  std::string msg_str = msg.dump() + "\n";

  std::lock_guard<std::mutex> lock(mutex_);
  for (int fd : clients_) {
    send(fd, msg_str.c_str(), msg_str.size(), MSG_NOSIGNAL);
  }
}
} // namespace si::rpc
