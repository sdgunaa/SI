#ifndef MANAGER_H
#define MANAGER_H

#include "session.h"
#include <iostream>
#include <map>
#include <memory>
#include <mutex>

using namespace std;

class Manager {
private:
  static const int MaxActiveSessions = 10;
  int sessionCount;
  mutex sessionMutex;
  map<int, shared_ptr<Session>> sessions;
  shared_ptr<Logger> logger;
  int generateSessionId() { return sessionCount += 1; }

public:
  Manager(shared_ptr<Logger> &log) : sessionCount(0), logger(log){};

  std::shared_ptr<Session> createSession(const std::string &userName) {
    lock_guard<mutex> lock(sessionMutex);
    if (sessionCount >= MaxActiveSessions) {
        logger->log(LogLevel::ERROR, "Cannot create session: maximum number of active sessions reached.", true);
      return nullptr;
    }
    int sessionId = generateSessionId();
    sessions[sessionId] = std::make_shared<Session>(userName, logger);
    return sessions[sessionId];
  }

  std::shared_ptr<Session> getSession(int sessionId) {
    lock_guard<mutex> lock(sessionMutex);
    auto it = sessions.find(sessionId);
    if (it != sessions.end()) {
      return it->second;
    }
    logger->log(LogLevel::ERROR, "Session Not Found", true);
    return nullptr;
  }
  void deleteSession(int sessionId) {
    lock_guard<mutex> lock(sessionMutex);
    auto it = sessions.find(sessionId);
    if (it != sessions.end()) {
      it->second->endSession();
      sessions.erase(it);
      logger->log(LogLevel::SUCCESS, "Session with ID " + to_string(sessionId) + " deleted.", false);
    } else {
        logger->log(LogLevel::ERROR, "Session not found.", true);
    }
  }

  void listActiveSessions() {
    lock_guard<mutex> lock(sessionMutex);
    std::cout << "Active Sessions: " << std::endl;
    for (const auto &pair : sessions) {
      if (pair.second->isActive()) {
        std::cout << "Session ID: " << pair.first
                  << ", User: " << pair.second->getUsername() << std::endl;
      }
    }
  }

  void clearSessions() {
    sessions.clear();
    logger->log(LogLevel::SUCCESS, "Sessions cleared", false);
  }
};
#endif