#ifndef DATA_EXECUTOR_H
#define DATA_EXECUTOR_H

#include "muduo/net/TcpConnection.h"
#include "muduo/base/Logging.h"

#include <map>
#include <mutex>
#include <condition_variable>

class DataExecutor {
    public:
        DataExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            :connections(conns), size(connections.size()) {}

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn, 
                               muduo::net::Buffer* buf, muduo::Timestamp time) = 0;

    protected:
        std::map<std::string, muduo::net::TcpConnectionPtr>& connections;
        size_t size;
        std::mutex mt;
        std::condition_variable cond;
};

#endif
