#ifndef DATA_SERVER_H
#define DATA_SERVER_H

#include "muduo/net/TcpClient.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"

#include "dataExecutor.h"

#include <mutex>
#include <condition_variable>

class DataServer {
    public:
        DataServer(muduo::net::EventLoop* loop,
                muduo::net::InetAddress& listenAddr,
                std::vector<muduo::net::InetAddress>& workers);

        void start();

    private:
        void onClientConnection(const muduo::net::TcpConnectionPtr& conn);
        void onClientMessage(const muduo::net::TcpConnectionPtr& conn, 
                             muduo::net::Buffer* buf,
                             muduo::Timestamp time);

        void onWorkerConnection(const muduo::net::TcpConnectionPtr& conn);
        void onWorkerMessage(const muduo::net::TcpConnectionPtr& conn, 
                             muduo::net::Buffer* buf,
                             muduo::Timestamp time);

        muduo::net::EventLoop* loop_;
        muduo::net::TcpServer server_;
        std::vector<muduo::net::InetAddress> workerAddrs;
        std::map<std::string, std::unique_ptr<muduo::net::TcpClient>> workers;
        std::map<std::string, muduo::net::TcpConnectionPtr> connections;

        std::mutex mt;
        std::condition_variable cond;
        size_t size;
        muduo::net::EventLoopThread workerLoopThread;
        DataExecutor* dataExecutor;
};

#endif
