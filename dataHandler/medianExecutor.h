#ifndef MEDIAN_EXECUTOR_H
#define MEDIAN_EXECUTOR_H

#include "dataExecutor.h"

#include "muduo/net/TcpConnection.h"

class MedianExecutor : public DataExecutor {
    public:
        MedianExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            : DataExecutor(conns) {
            }

        int64_t execute();

        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buffer, muduo::Timestamp time);
    private:
};

#endif
