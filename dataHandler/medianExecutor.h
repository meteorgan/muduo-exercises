#ifndef MEDIAN_EXECUTOR_H
#define MEDIAN_EXECUTOR_H

#include "dataExecutor.h"

#include "muduo/net/TcpConnection.h"

class MedianExecutor : public DataExecutor {
    public:
        MedianExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            : DataExecutor(conns), workingSize(connections.size()) {
                for(auto& conn : connections) {
                    std::string id(conn.second->peerAddress().toIpPort().c_str());
                    workerBuffers[id] = std::vector<int64_t>();
                }
            }

        int64_t execute();

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buffer, muduo::Timestamp time);
    private:
        size_t workingSize;
        std::map<std::string, std::vector<int64_t>> workerBuffers;
};

#endif
