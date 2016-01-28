#include "Request.h"
#include "Queen.h"

Request::Request(std::string clientId, std::string requestId, int size, bool computeSolutions)
  : clientId(clientId), taskId(requestId), requestSize(size),
    requestId(clientId + "-" + taskId), computeSolutions(computeSolutions) {
}

const std::map<int, std::vector<int>>& Request::splitRequest() {
  if(subtasks.empty()) {
    int subId = 0;
    for(int i = 0; i < requestSize; ++i) {
      std::list<std::vector<int>> twoRows = put_n_row(std::vector<int>(1, i), requestSize);
      for(auto &task : twoRows) {
        subtaskIds.push_back(subId);
        subtasks[subId++] = task;
      }
    }
  }

  return subtasks;
}
