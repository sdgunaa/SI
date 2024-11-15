#ifndef SESSION_H
#define SESSION_H

#include "command.h"
#include "logger.h"
#include "task.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

using namespace std;

enum class SessionStatus { ACTIVE, TERMINATED, SUSPENDED };

class Session {

public:
  Session(const std::string &userName, optional<shared_ptr<Logger>> log = nullptr)
      : UserName(userName), start_time(std::chrono::steady_clock::now()),
        last_active_time(std::time(nullptr)), status(SessionStatus::ACTIVE), logger(log) {}

  std::string getUsername() { return UserName; }
  bool isActive() const {
    if (status == SessionStatus::ACTIVE)
      return true;
    return false;
  }
  bool isSuspended() const {
    if (status == SessionStatus::SUSPENDED)
      return true;
    return false;
  }
  bool isTerminated() const {
    if (status == SessionStatus::TERMINATED)
      return true;
    return false;
  }
  time_t getLastActiveTime() const { return last_active_time; }
  void updateLastActiveTime() { last_active_time = std::time(nullptr); }
  void clearCommandHistory() 
  { 
    commands->clear(); 
    if (logger)
    {
        logger->get()->log(LogLevel::INFO, "Command history cleared", false);
    }
}
  bool ResumeSession() {
    if (isSuspended()) {
      status = SessionStatus::ACTIVE;
      updateLastActiveTime();
        if (logger)
        {
            logger->get()->log(LogLevel::INFO, "Session Resumed", false);
        }
      return true;
    }
    return false;
  }
  bool SuspendSession() {
    if (isActive()) {
      status = SessionStatus::SUSPENDED;
      updateLastActiveTime();
      if (logger)
        {
            logger->get()->log(LogLevel::INFO, "Session Suspended", false);
        }
      return true;
    }
    return false;
  }
  bool TerminateSession() {
    if (!isTerminated()) {
      status = SessionStatus::TERMINATED;
      end_time = std::chrono::steady_clock::now();
      updateLastActiveTime();
      if (logger)
        {
            logger->get()->log(LogLevel::INFO, "Session Terminated", false);
        }
      return true;
    }
    return false;
  }
  void addCommand(const Command &cmd) { commands->push_back(cmd); }
  void addTask(const Task &task) 
  { 
    tasks->push_back(task);
    if (logger)
    {
        logger->get()->log(LogLevel::INFO, "Task added: " + task.getTaskName(), false);
    }
    }
  void endSession() {
    if (isTerminated()) {
      if (logger)
        {
            logger->get()->log(LogLevel::ERROR, "Session Already ended: " + UserName, false);
            return;
        }
        cerr << "Session has already been terminated." << endl;
      return;
    }
    TerminateSession();
    if (logger)
    {
        logger->get()->log(LogLevel::INFO, "Session ended: " + UserName, false);
        return;
    }
    cout << "Session for user " << UserName << " ended." << endl;
    
  }
  void printSessionInfo() const {
    cout << "Username: " << UserName << endl;
    cout << "Active: " << (isActive() ? "Yes" : "No") << endl;
    cout << "Start Time: " << formatTime(start_time) << endl;
    
  }

private:
  std::string UserName;
  optional<vector<Command>> commands;
  optional<vector<Task>> tasks;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> end_time;
  time_t last_active_time;
  SessionStatus status;
  optional<shared_ptr<Logger>> logger;

  string formatTime(const std::chrono::time_point<std::chrono::steady_clock>
                        &time_point) const {
    auto time_in_seconds =
        chrono::duration_cast<chrono::seconds>(time_point.time_since_epoch())
            .count();
    time_t time = static_cast<time_t>(time_in_seconds);
    return ctime(&time);
  }
};

#endif