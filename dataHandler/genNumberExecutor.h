#ifndef GEN_NUMBER_EXECUTOR_H
#define GEN_NUMBER_EXECUTOR_H

#include "dataExecutor.h"

class GenNumberExecutor : public DataExecutor {
    public:
        GenNumberExecutor(std::map<std::string, muduo::net::TcpConnectionPtr>& conns)
            : DataExecutor(conns) {}

        void execute(int64_t number, char mode);

        virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buf, muduo::Timestamp time);
};

#endif
