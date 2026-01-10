#include "si/shell/workflow_engine.hpp"
#include "si/foundation/logging.hpp"
#include <regex>
#include <sstream>

namespace si::shell {

WorkflowEngine &WorkflowEngine::instance() {
  static WorkflowEngine inst;
  return inst;
}

std::string WorkflowEngine::save_workflow(const Workflow &workflow) {
  std::lock_guard<std::mutex> lock(mutex_);
  // If ID empty, generate? For now assume provided or use name hash
  std::string id = workflow.id;
  if (id.empty())
    id = workflow.name; // Simplification

  workflows_[id] = workflow;
  SI_LOG_INFO("Saved Workflow: {}", workflow.name);
  return id;
}

std::optional<Workflow> WorkflowEngine::get_workflow(const std::string &id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (workflows_.count(id))
    return workflows_[id];
  return std::nullopt;
}

std::vector<Workflow>
WorkflowEngine::list_workflows(const std::string &tag_filter) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Workflow> result;
  for (const auto &[id, w] : workflows_) {
    if (tag_filter.empty()) {
      result.push_back(w);
    } else {
      for (const auto &t : w.tags) {
        if (t == tag_filter) {
          result.push_back(w);
          break;
        }
      }
    }
  }
  return result;
}

std::string WorkflowEngine::render_command(
    const std::string &workflow_id,
    const std::map<std::string, std::string> &params) {
  auto w_opt = get_workflow(workflow_id);
  if (!w_opt)
    return "";

  std::string result = w_opt->command_template;

  // Replace {{key}} with value
  for (const auto &[key, val] : params) {
    std::string pattern = "\\{\\{" + key + "\\}\\}";
    std::regex re(pattern);
    result = std::regex_replace(result, re, val);
  }

  // Check for remaining {{...}} which implies missing params?
  // For now, leave them or warn.
  return result;
}

void WorkflowEngine::load_from_directory(const std::string &path) {
  // Stub: Would iterate std::filesystem and parse JSON/YAML
  SI_LOG_INFO("Loading workflows from {}", path);
}

} // namespace si::shell
