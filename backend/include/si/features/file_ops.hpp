#pragma once
#include <optional>
#include <string>
#include <vector>

namespace si::features {

struct FileSummary {
  std::string summary;
  std::string language;
  int line_count = 0;
};

struct SearchResult {
  std::string file_path;
  int line_number;
  std::string context;
  float relevance;
};

class FileOperations {
public:
  FileOperations();
  ~FileOperations();

  // Summarize a file using AI
  std::optional<FileSummary> summarize(const std::string &file_path);

  // Explain what code does
  std::optional<std::string> explain(const std::string &file_path,
                                     int start_line = 1, int end_line = -1);

  // Answer questions about a file
  std::optional<std::string> ask(const std::string &file_path,
                                 const std::string &question);

private:
  std::string read_file(const std::string &path, int max_lines = 500);
  std::string detect_language(const std::string &path);
};

} // namespace si::features
