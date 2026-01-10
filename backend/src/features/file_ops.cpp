#include "si/features/file_ops.hpp"
#include "si/ai/gateway.hpp"
#include "si/foundation/logging.hpp"
#include <filesystem>
#include <fstream>
#include <si/nlohmann/json.hpp>
#include <regex>
#include <sstream>

namespace si::features {

FileOperations::FileOperations() {}
FileOperations::~FileOperations() {}

std::string FileOperations::read_file(const std::string &path, int max_lines) {
  std::ifstream file(path);
  if (!file)
    return "";

  std::ostringstream content;
  std::string line;
  int count = 0;
  while (std::getline(file, line) && count < max_lines) {
    content << line << "\n";
    count++;
  }
  return content.str();
}

std::string FileOperations::detect_language(const std::string &path) {
  std::filesystem::path p(path);
  auto ext = p.extension().string();
  if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".cc")
    return "C++";
  if (ext == ".py")
    return "Python";
  if (ext == ".js" || ext == ".ts")
    return "JavaScript";
  if (ext == ".rs")
    return "Rust";
  if (ext == ".go")
    return "Go";
  if (ext == ".java")
    return "Java";
  if (ext == ".sh" || ext == ".bash")
    return "Shell";
  if (ext == ".md")
    return "Markdown";
  if (ext == ".json")
    return "JSON";
  if (ext == ".toml")
    return "TOML";
  if (ext == ".yaml" || ext == ".yml")
    return "YAML";
  return "Text";
}

std::optional<FileSummary>
FileOperations::summarize(const std::string &file_path) {
  auto &gateway = ai::AIGateway::instance();
  if (!gateway.is_available())
    return std::nullopt;

  auto content = read_file(file_path, 200);
  if (content.empty())
    return std::nullopt;

  std::string prompt =
      "Summarize this file in 2-3 sentences. Output ONLY the summary.\n\n"
      "File: " +
      file_path + "\n```\n" + content + "\n```\nSummary:";

  ai::CompletionRequest req;
  req.prompt = prompt;
  req.max_tokens = 256;
  req.temperature = 0.3f;

  auto response = gateway.complete(req);
  if (!response.success)
    return std::nullopt;

  FileSummary summary;
  summary.summary = response.content;
  summary.language = detect_language(file_path);

  // Count lines
  std::ifstream file(file_path);
  std::string line;
  while (std::getline(file, line))
    summary.line_count++;

  return summary;
}

std::optional<std::string> FileOperations::explain(const std::string &file_path,
                                                   int start_line,
                                                   int end_line) {
  auto &gateway = ai::AIGateway::instance();
  if (!gateway.is_available())
    return std::nullopt;

  auto content = read_file(file_path, 300);
  if (content.empty())
    return std::nullopt;

  std::string prompt = "Explain what this code does. Be concise.\n\n```\n" +
                       content + "\n```\nExplanation:";

  ai::CompletionRequest req;
  req.prompt = prompt;
  req.max_tokens = 512;
  req.temperature = 0.3f;

  auto response = gateway.complete(req);
  return response.success ? std::optional(response.content) : std::nullopt;
}

std::optional<std::string> FileOperations::ask(const std::string &file_path,
                                               const std::string &question) {
  auto &gateway = ai::AIGateway::instance();
  if (!gateway.is_available())
    return std::nullopt;

  auto content = read_file(file_path, 300);
  if (content.empty())
    return std::nullopt;

  std::string prompt = "Answer the question about this file. Be concise.\n\n"
                       "File:\n```\n" +
                       content + "\n```\n\nQuestion: " + question + "\nAnswer:";

  ai::CompletionRequest req;
  req.prompt = prompt;
  req.max_tokens = 512;
  req.temperature = 0.3f;

  auto response = gateway.complete(req);
  return response.success ? std::optional(response.content) : std::nullopt;
}

} // namespace si::features
