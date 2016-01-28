#ifndef QUEEN_REQUEST_H
#define QUEEN_REQUEST_H

#include <string>
#include <vector>
#include <map>
#include <list>

class Request {
  public:
    Request(std::string clientId, std::string requestId, int size);

    Request(Request&) = delete;
    Request& operator=(Request&) = delete;

    std::string getId() const {
      return requestId;
    }

    std::string getClientId() const {
      return clientId;
    }

    std::string getTaskId() const {
      return taskId;
    }

    int getRequestSize() const {
      return requestSize;
    }

    const std::map<int, std::vector<int>>& splitRequest();

    void setSubTaskSolutiosNumber(int subId, int number) {
      subtaskSolutionsNumber[subId] = number;
    }

    void addSubTaskSolution(int subId, const std::vector<int>& solution) {
      subtaskSolutions[subId].push_back(solution);
    }

    bool isSubTaskFinish(int subId) {
      return  (subtaskSolutionsNumber.find(subId) != subtaskSolutionsNumber.end())
        && (subtaskSolutionsNumber[subId] == static_cast<int>(subtaskSolutions[subId].size()));
    }

    bool isTaskFinish() {
      for(auto &id : subtaskIds) {
        if(!isSubTaskFinish(id)) 
          return false;
      }

      return true;
    }

    std::list<std::vector<int>> getAllSolutions() const {
      std::list<std::vector<int>> solutions;
      for(auto &subSolutions: subtaskSolutions) {
        solutions.insert(solutions.end(), subSolutions.second.begin(), subSolutions.second.end());
      }

      return solutions;
    }

  private:
    std::string clientId;
    std::string taskId;
    int requestSize;
    std::string requestId;
    std::vector<int> subtaskIds;
    std::map<int, std::vector<int>> subtasks;
    std::map<int, int> subtaskSolutionsNumber;
    std::map<int, std::list<std::vector<int>>> subtaskSolutions;
};

#endif
