#ifndef SORT_EXECUTOR_H
#define SORT_EXECUTOR_H

#include "dataExecutor.h"

#include "muduo/net/TcpConnection.h"

#include <list>
#include <set>

class SortExecutor : public DataExecutor {
    public:
        SortExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            : DataExecutor(conns), number(0), workingSize(0), sendNumber(0) {
                for(auto& conn : connections) {
                    std::string id(conn.first);
                    workerBuffers[id] = std::list<std::string>();
                    notFinishedWorkers.insert(id);
                    workerStatus[id] = false;
                }
                currentOutput = workerStatus.begin();
            }

        void execute();

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buf, muduo::Timestamp time);

    private:
        void mergeNumbers();

        const int batchSize = 1024;
        int64_t number;
        size_t workingSize;
        int64_t sendNumber;
        std::map<std::string, std::list<std::string>> workerBuffers;
        std::list<std::string> outputBuffer;
        std::set<std::string> notFinishedWorkers;
        std::map<std::string, bool> workerStatus;
        std::map<std::string, bool>::iterator currentOutput;
};

#endif
