#pragma once

#include "si/shell/workflow.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace si::shell {

class WorkflowEngine {
public:
  static WorkflowEngine &instance();

  // CRUD
  std::string save_workflow(const Workflow &workflow);
  std::optional<Workflow> get_workflow(const std::string &id);
  std::vector<Workflow> list_workflows(const std::string &tag_filter = "");

  // Core Logic
  std::string render_command(const std::string &workflow_id,
                             const std::map<std::string, std::string> &params);

  // Persistence
  void load_from_directory(const std::string &path);

private:
  WorkflowEngine() = default;

  std::map<std::string, Workflow> workflows_;
  std::mutex mutex_;
};

} // namespace si::shell
