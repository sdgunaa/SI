#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include "command.h"

using namespace std;

enum class TaskStatus { PENDING, PAUSED, COMPLETED, CANCELLED };
enum class TaskPriority { Low, Medium, High };

class Task
{
private:
    std::string taskName;
    TaskStatus status;
    TaskPriority priority;
    string description;
    Command command;
    bool isBackgroundTask;

public:

    Task(const string& name, const string& desc, const Command& cmd, bool isBg = false) 
        : taskName(name), description(desc), command(cmd) , isBackgroundTask(isBg) {}

    void execute()
    {
        if (!command.isExecuted()) {
            command.execute();
        } else {
            cerr << "Command has already been executed." << endl;
        }
    }

    bool isCompleted() const { return status == TaskStatus::COMPLETED || status == TaskStatus::CANCELLED; }
    const string& getTaskName() const { return taskName; }

    void printTaskInfo()
    {
        cout << "Task Name: " << taskName << endl;
        cout << "Description: " << description << endl;
        cout << "Command: " << command.toString() << endl;
        cout << "Background Task: " << (isBackgroundTask? "Yes" : "No") << endl;
    }
};


#endif