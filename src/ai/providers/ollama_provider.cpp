#include "ai/providers/ollama_provider.hpp"
#include "foundation/logging.hpp"
#include <chrono>
#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace si::ai {

OllamaProvider::OllamaProvider(const std::string &host,
                               const std::string &model_name)
    : host_(host), model_name_(model_name) {}

OllamaProvider::~OllamaProvider() { shutdown(); }

bool OllamaProvider::initialize() {
  SI_LOG_INFO("Initializing Ollama provider...");
  SI_LOG_INFO("  Host: {}", host_);
  SI_LOG_INFO("  Model: {}", model_name_);

  // Check if Ollama server is running
  if (!check_server_health()) {
    SI_LOG_ERROR("Ollama server not reachable at {}", host_);
    return false;
  }

  // Check if model exists
  if (!check_model_exists()) {
    SI_LOG_ERROR("Model '{}' not found. Run: ollama pull {}", model_name_,
                 model_name_);
    return false;
  }

  initialized_ = true;
  SI_LOG_INFO("Ollama provider initialized successfully");
  return true;
}

bool OllamaProvider::is_available() const { return initialized_; }

ModelInfo OllamaProvider::get_model_info() const {
  ModelInfo info;
  info.name = model_name_;
  info.type = "ollama";
  info.context_window = 2048; // Default, can be queried from Ollama
  info.supports_gpu = true;   // Ollama handles GPU automatically
  info.loaded = initialized_;
  return info;
}

CompletionResponse OllamaProvider::complete(const CompletionRequest &request) {
  auto start = std::chrono::high_resolution_clock::now();
  CompletionResponse response;

  if (!initialized_) {
    response.success = false;
    response.error_message = "Provider not initialized";
    return response;
  }

  // Build request JSON
  json req_json = {{"model", model_name_},
                   {"prompt", request.prompt},
                   {"stream", false},
                   {"options",
                    {{"temperature", request.temperature},
                     {"num_predict", request.max_tokens}}}};

  if (!request.stop_sequences.empty()) {
    req_json["options"]["stop"] = request.stop_sequences;
  }

  // Make request
  std::string result = make_request("/api/generate", req_json.dump());

  if (result.empty()) {
    response.success = false;
    response.error_message = "Empty response from Ollama";
    return response;
  }

  try {
    json resp_json = json::parse(result);
    response.content = resp_json["response"].get<std::string>();
    response.success = true;

    auto end = std::chrono::high_resolution_clock::now();
    response.latency_ms =
        std::chrono::duration<float, std::milli>(end - start).count();

    SI_LOG_DEBUG("Ollama completion: {:.2f}ms, {} chars", response.latency_ms,
                 response.content.length());

  } catch (const json::exception &e) {
    response.success = false;
    response.error_message = std::string("JSON parse error: ") + e.what();
    SI_LOG_ERROR(response.error_message);
  }

  return response;
}

CompletionResponse OllamaProvider::stream(const CompletionRequest &request,
                                          TokenCallback callback) {
  auto start = std::chrono::high_resolution_clock::now();
  CompletionResponse response;

  if (!initialized_) {
    response.success = false;
    response.error_message = "Provider not initialized";
    return response;
  }

  // Build request JSON
  json req_json = {{"model", model_name_},
                   {"prompt", request.prompt},
                   {"stream", true},
                   {"options",
                    {{"temperature", request.temperature},
                     {"num_predict", request.max_tokens}}}};

  // Make streaming request
  std::string full_response =
      make_request("/api/generate", req_json.dump(), true, callback);

  response.content = full_response;
  response.success = !full_response.empty();

  auto end = std::chrono::high_resolution_clock::now();
  response.latency_ms =
      std::chrono::duration<float, std::milli>(end - start).count();

  return response;
}

void OllamaProvider::shutdown() {
  initialized_ = false;
  SI_LOG_INFO("Ollama provider shutdown");
}

bool OllamaProvider::check_server_health() {
  try {
    httplib::Client client(host_);
    client.set_connection_timeout(2, 0); // 2 seconds

    auto res = client.Get("/");
    return res && res->status == 200;
  } catch (const std::exception &e) {
    SI_LOG_ERROR("Health check failed: {}", e.what());
    return false;
  }
}

bool OllamaProvider::check_model_exists() {
  try {
    httplib::Client client(host_);

    // Get list of models
    auto res = client.Get("/api/tags");
    if (!res || res->status != 200) {
      return false;
    }

    json models = json::parse(res->body);
    for (const auto &model : models["models"]) {
      std::string name = model["name"].get<std::string>();
      // Remove tag suffix if present
      size_t colon_pos = name.find(':');
      std::string base_name =
          (colon_pos != std::string::npos) ? name.substr(0, colon_pos) : name;

      if (name == model_name_ || base_name == model_name_) {
        SI_LOG_INFO("Found model: {}", name);
        return true;
      }
    }

    return false;
  } catch (const std::exception &e) {
    SI_LOG_ERROR("Model check failed: {}", e.what());
    return false;
  }
}

std::string OllamaProvider::make_request(const std::string &endpoint,
                                         const std::string &json_body,
                                         bool stream, TokenCallback callback) {
  try {
    httplib::Client client(host_);
    client.set_connection_timeout(30, 0);
    client.set_read_timeout(60, 0);

    httplib::Headers headers = {{"Content-Type", "application/json"}};

    if (stream && callback) {
      std::string full_response;

      auto res =
          client.Post(endpoint.c_str(), headers, json_body, "application/json",
                      [&](const char *data, size_t data_length) {
                        std::string chunk(data, data_length);

                        try {
                          json resp = json::parse(chunk);
                          if (resp.contains("response")) {
                            std::string token =
                                resp["response"].get<std::string>();
                            full_response += token;
                            if (callback) {
                              callback(token);
                            }
                          }
                        } catch (...) {
                          // Ignore malformed chunks
                        }

                        return true;
                      });

      return full_response;
    } else {
      auto res =
          client.Post(endpoint.c_str(), headers, json_body, "application/json");
      if (res && res->status == 200) {
        return res->body;
      }
    }

  } catch (const std::exception &e) {
    SI_LOG_ERROR("Request failed: {}", e.what());
  }

  return "";
}

} // namespace si::ai
