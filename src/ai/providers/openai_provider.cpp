#include "ai/providers/openai_provider.hpp"
#include "foundation/logging.hpp"
#include <chrono>
#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace si::ai {

OpenAIProvider::OpenAIProvider(const std::string &api_key,
                               const std::string &model_name)
    : api_key_(api_key), model_name_(model_name) {}

OpenAIProvider::~OpenAIProvider() { shutdown(); }

bool OpenAIProvider::initialize() {
  SI_LOG_INFO("Initializing OpenAI provider...");

  if (api_key_.empty()) {
    SI_LOG_ERROR("OpenAI API key is empty");
    return false;
  }

  initialized_ = true;
  return true;
}

bool OpenAIProvider::is_available() const {
  return initialized_ && !api_key_.empty();
}

ModelInfo OpenAIProvider::get_model_info() const {
  ModelInfo info;
  info.name = model_name_;
  info.type = "openai";
  info.context_window = 8192; // Approximate for GPT-4
  info.supports_gpu = false;  // Cloud based
  info.loaded = initialized_;
  return info;
}

CompletionResponse OpenAIProvider::complete(const CompletionRequest &request) {
  auto start = std::chrono::high_resolution_clock::now();
  CompletionResponse response;

  if (!initialized_) {
    response.success = false;
    response.error_message = "Provider not initialized";
    return response;
  }

  // Build request JSON
  json req_json = {
      {"model", model_name_},
      {"messages", {{{"role", "user"}, {"content", request.prompt}}}},
      {"temperature", request.temperature},
      {"max_tokens", request.max_tokens},
      {"stream", false}};

  if (!request.stop_sequences.empty()) {
    req_json["stop"] = request.stop_sequences;
  }

  // Make request
  std::string result = make_request("/v1/chat/completions", req_json.dump());

  if (result.empty()) {
    response.success = false;
    response.error_message = "Empty response from OpenAI";
    return response;
  }

  try {
    json resp_json = json::parse(result);

    if (resp_json.contains("error")) {
      response.success = false;
      response.error_message = resp_json["error"]["message"].get<std::string>();
      SI_LOG_ERROR("OpenAI API error: {}", response.error_message);
      return response;
    }

    response.content =
        resp_json["choices"][0]["message"]["content"].get<std::string>();
    response.tokens_used = resp_json["usage"]["total_tokens"].get<int>();
    response.success = true;

    auto end = std::chrono::high_resolution_clock::now();
    response.latency_ms =
        std::chrono::duration<float, std::milli>(end - start).count();

    SI_LOG_DEBUG("OpenAI completion: {:.2f}ms, {} tokens", response.latency_ms,
                 response.tokens_used);

  } catch (const json::exception &e) {
    response.success = false;
    response.error_message = std::string("JSON parse error: ") + e.what();
    SI_LOG_ERROR(response.error_message);
  }

  return response;
}

CompletionResponse OpenAIProvider::stream(const CompletionRequest &request,
                                          TokenCallback callback) {
  auto start = std::chrono::high_resolution_clock::now();
  CompletionResponse response;

  if (!initialized_) {
    response.success = false;
    response.error_message = "Provider not initialized";
    return response;
  }

  // Build request JSON
  json req_json = {
      {"model", model_name_},
      {"messages", {{{"role", "user"}, {"content", request.prompt}}}},
      {"temperature", request.temperature},
      {"max_tokens", request.max_tokens},
      {"stream", true}};

  // Make streaming request
  std::string full_response =
      make_request("/v1/chat/completions", req_json.dump(), true, callback);

  response.content = full_response;
  response.success = !full_response.empty();

  auto end = std::chrono::high_resolution_clock::now();
  response.latency_ms =
      std::chrono::duration<float, std::milli>(end - start).count();

  return response;
}

void OpenAIProvider::shutdown() { initialized_ = false; }

std::string OpenAIProvider::make_request(const std::string &endpoint,
                                         const std::string &json_body,
                                         bool stream, TokenCallback callback) {
  try {
    httplib::SSLClient client("api.openai.com");
    client.set_connection_timeout(30, 0);
    client.set_read_timeout(60, 0);

    httplib::Headers headers = {{"Content-Type", "application/json"},
                                {"Authorization", "Bearer " + api_key_}};

    if (stream && callback) {
      std::string full_response;

      auto res = client.Post(
          endpoint.c_str(), headers, json_body, "application/json",
          [&](const char *data, size_t data_length) {
            std::string chunk(data, data_length);

            // SSE parsing (simplified)
            // Data comes as "data: {json}\n\n"
            std::istringstream stream(chunk);
            std::string line;
            while (std::getline(stream, line)) {
              if (line.rfind("data: ", 0) == 0) {
                std::string json_str = line.substr(6);
                if (json_str == "[DONE]")
                  continue;

                try {
                  json resp = json::parse(json_str);
                  if (resp["choices"][0]["delta"].contains("content")) {
                    std::string token = resp["choices"][0]["delta"]["content"]
                                            .get<std::string>();
                    full_response += token;
                    if (callback) {
                      callback(token);
                    }
                  }
                } catch (...) {
                  // Ignore
                }
              }
            }

            return true;
          });

      return full_response;
    } else {
      auto res =
          client.Post(endpoint.c_str(), headers, json_body, "application/json");
      if (res && res->status == 200) {
        return res->body;
      } else if (res) {
        SI_LOG_ERROR("OpenAI request failed: {} {}", res->status, res->body);
      }
    }

  } catch (const std::exception &e) {
    SI_LOG_ERROR("Request failed: {}", e.what());
  }

  return "";
}

} // namespace si::ai
