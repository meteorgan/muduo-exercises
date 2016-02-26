#ifndef SORT_EXECUTOR_H
#define SORT_EXECUTOR_H

#include "dataExecutor.h"

#include "muduo/net/TcpConnection.h"

#include <list>
#include <set>

class SortExecutor : public DataExecutor {
    public:
        SortExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            : DataExecutor(conns), number(0), total(0), workingSize(0),
            sendNumber(0) {
                for(auto& conn : connections) {
                    std::string id(conn.second->peerAddress().toIpPort().c_str());
                    workerBuffers[id] = std::list<std::string>();
                    notFinishedWorkers.insert(id);
                }
                currentOutput = outputBuffer.begin();
            }

        void execute();

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buf, muduo::Timestamp time);

    private:
        void mergeNumbers();

        const int batchSize = 1014;
        int64_t number;
        int total;
        int workingSize;
        int64_t sendNumber;
        std::map<std::string, std::list<std::string>> workerBuffers;
        std::list<std::string> outputBuffer;
        std::set<std::string> notFinishedWorkers;
        std::list<std::string>::iterator currentOutput;
};

#endif
