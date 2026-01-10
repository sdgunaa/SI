#include "si/ai/providers/openai_provider.hpp"
#include "si/foundation/logging.hpp"
#include <chrono>
#include <httplib.h>
#include <si/nlohmann/json.hpp>

using json = nlohmann::json;

namespace si::ai {

OpenAIProvider::OpenAIProvider(const std::string &api_key,
                               const std::string &model_name,
                               const std::string &base_url)
    : api_key_(api_key), model_name_(model_name), base_url_(base_url) {}

OpenAIProvider::~OpenAIProvider() { shutdown(); }

bool OpenAIProvider::initialize() {
  SI_LOG_INFO("Initializing OpenAI provider ({})...", base_url_);

  if (api_key_.empty() && base_url_.find("openai.com") != std::string::npos) {
    SI_LOG_ERROR("OpenAI API key is empty");
    return false;
  }

  initialized_ = true;
  return true;
}

bool OpenAIProvider::is_available() const {
  return initialized_; // Allow usage if initialized, even if key is empty (for
                       // local LLMs)
}

ModelInfo OpenAIProvider::get_model_info() const {
  ModelInfo info;
  info.name = model_name_;
  info.type = "openai";
  info.context_window = 8192; // Approximate/Default
  info.supports_gpu = false;
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

  json req_json = {
      {"model", model_name_},
      {"messages", {{{"role", "user"}, {"content", request.prompt}}}},
      {"temperature", request.temperature},
      {"max_tokens", request.max_tokens},
      {"stream", false}};

  if (!request.stop_sequences.empty()) {
    req_json["stop"] = request.stop_sequences;
  }

  std::string result = make_request("/v1/chat/completions", req_json.dump());

  if (result.empty()) {
    response.success = false;
    response.error_message = "Empty response from provider";
    return response;
  }

  try {
    json resp_json = json::parse(result);

    if (resp_json.contains("error")) {
      response.success = false;
      if (resp_json["error"].is_object() &&
          resp_json["error"].contains("message")) {
        response.error_message =
            resp_json["error"]["message"].get<std::string>();
      } else {
        response.error_message = resp_json["error"].dump();
      }
      SI_LOG_ERROR("API error: {}", response.error_message);
      return response;
    }

    if (resp_json.contains("choices") && !resp_json["choices"].empty()) {
      response.content =
          resp_json["choices"][0]["message"]["content"].get<std::string>();
      if (resp_json.contains("usage")) {
        response.tokens_used =
            resp_json["usage"]["total_tokens"].value("total_tokens", 0);
      }
      response.success = true;
    } else {
      response.success = false;
      response.error_message = "Invalid response format";
    }

    auto end = std::chrono::high_resolution_clock::now();
    response.latency_ms =
        std::chrono::duration<float, std::milli>(end - start).count();

    SI_LOG_DEBUG("Completion: {:.2f}ms, {} tokens", response.latency_ms,
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

  json req_json = {
      {"model", model_name_},
      {"messages", {{{"role", "user"}, {"content", request.prompt}}}},
      {"temperature", request.temperature},
      {"max_tokens", request.max_tokens},
      {"stream", true}};

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

template <typename ClientType>
std::string run_request(ClientType &client, const std::string &endpoint,
                        const httplib::Headers &headers,
                        const std::string &json_body, bool stream,
                        TokenCallback callback) {
  client.set_connection_timeout(30, 0);
  client.set_read_timeout(60, 0);

  if (stream && callback) {
    std::string full_response;
    auto res = client.Post(
        endpoint.c_str(), headers, json_body, "application/json",
        [&](const char *data, size_t data_length) {
          std::string chunk(data, data_length);
          std::istringstream stream(chunk);
          std::string line;
          while (std::getline(stream, line)) {
            if (line.rfind("data: ", 0) == 0) {
              std::string json_str = line.substr(6);
              if (json_str == "[DONE]")
                continue;
              try {
                // Handle possible trailing newlines/carriage returns
                if (!json_str.empty() && json_str.back() == '\r')
                  json_str.pop_back();

                json resp = json::parse(json_str);
                if (resp.contains("choices") && !resp["choices"].empty() &&
                    resp["choices"][0].contains("delta") &&
                    resp["choices"][0]["delta"].contains("content")) {

                  std::string token =
                      resp["choices"][0]["delta"]["content"].get<std::string>();
                  full_response += token;
                  if (callback)
                    callback(token);
                }
              } catch (...) {
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
      SI_LOG_ERROR("Request failed: {} {}", res->status, res->body);
    }
  }
  return "";
}

std::string OpenAIProvider::make_request(const std::string &endpoint,
                                         const std::string &json_body,
                                         bool stream, TokenCallback callback) {
  try {
    httplib::Headers headers = {{"Content-Type", "application/json"},
                                {"Authorization", "Bearer " + api_key_}};

    bool is_ssl = base_url_.find("https://") == 0;
    std::string host = base_url_;

    // Naive host parsing
    if (is_ssl) {
      host = host.substr(8);
      if (host.back() == '/')
        host.pop_back();
      httplib::SSLClient client(host.c_str());
      return run_request(client, endpoint, headers, json_body, stream,
                         callback);
    } else {
      if (host.find("http://") == 0)
        host = host.substr(7);
      if (host.back() == '/')
        host.pop_back();
      httplib::Client client(base_url_.c_str()); // Client can take full URL
      return run_request(client, endpoint, headers, json_body, stream,
                         callback);
    }

  } catch (const std::exception &e) {
    SI_LOG_ERROR("Request failed: {}", e.what());
  }

  return "";
}

} // namespace si::ai
