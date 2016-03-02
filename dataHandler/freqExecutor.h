#ifndef FREQ_EXECUTOR_H
#define FREQ_EXECUTOR_H

#include "dataExecutor.h"

#include "muduo/net/TcpConnection.h"

#include <set>
#include <list>
#include <queue>
#include <functional>

class FreqExecutor : public DataExecutor {
    public:
        FreqExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            : DataExecutor(conns), workingSize(0) {
                for(auto& conn : conns) {
                    std::string id(conn.first);
                    workerBuffers[id] = std::list<std::pair<int64_t, int64_t>>();
                    notFinishedWorkers.insert(id);
                    workerStatus[id] = false;
                }
            }

        std::vector<std::pair<int64_t, int64_t>> execute();

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buf, muduo::Timestamp time);
    private:
        void mergeFreqs();

        size_t workingSize;
        std::priority_queue<std::pair<int64_t, int64_t>, 
                            std::vector<std::pair<int64_t, int64_t>>, 
                            std::greater<std::pair<int64_t, int64_t>>> topFreqs;
        const int batchSize = 1024;
        std::map<std::string, std::list<std::pair<int64_t, int64_t>>> workerBuffers;
        std::set<std::string> notFinishedWorkers;
        std::map<std::string, bool> workerStatus;
};

#endif
