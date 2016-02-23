#ifndef AVERAGE_EXECUTOR_H
#define AVERAGE_EXECUTOR_H

#include "dataExecutor.h"

class AverageExecutor : public DataExecutor {
    public:
        AverageExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            :DataExecutor(conns), sum(0), number(0) {}

        double execute();

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buf, muduo::Timestamp time);

    private:
        int64_t sum;
        int64_t number;
};

#endif
