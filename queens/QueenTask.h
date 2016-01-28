#ifndef QUEEN_TASK_H
#define QUEEN_TASK_H

#include <string>
#include <vector>

class QueenTask {
  public:
    QueenTask(std::string clientId, std::string taskId, 
              std::string subTaskId, int size, std::vector<int> &p, bool computeSolutions)
      : clientId(clientId), taskId(taskId), subTaskId(subTaskId), 
        size(size), pos(p), computeSolutions(computeSolutions) {}

    const std::string& getClientId() const { return clientId; }
    const std::string& getTaskId() const { return taskId; }
    const std::string& getSubTaskId() const { return subTaskId; }
    int   getSize() const { return size; }
    const std::vector<int>& getPos() const { return pos; }
    bool isComputeSolutions() const { return computeSolutions; }

  private:
    std::string clientId;
    std::string taskId;
    std::string subTaskId;
    int size;
    std::vector<int> pos;
    bool computeSolutions;
};

#endif
