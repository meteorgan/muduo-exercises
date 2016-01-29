#ifndef QUEEN_SERVER_H
#define QUEEN_SERVER_H

#include "muduo/base/Thread.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/net/TcpServer.h"

#include "Request.h"

#include <map>

class QueenServer {
  public:
    QueenServer(muduo::net::EventLoop* loop, 
        muduo::net::InetAddress& listenAddr);

    void start();
  private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp time);

    void processRequest(const muduo::net::TcpConnectionPtr&, const std::string);
    void solveQueen(const muduo::net::TcpConnectionPtr&, std::string, int);
    void dispatchTask(std::shared_ptr<Request>& request);
    void sendSolutionToClient(std::shared_ptr<Request>& request);
    std::string findLeastLoadWorker();

    muduo::net::TcpServer server;
    int index;
    std::map<std::string, muduo::net::TcpConnectionPtr> clients;
    std::map<std::string, muduo::net::TcpConnectionPtr> workers;
    std::map<std::string, int> activeRequests;
    std::map<std::string, std::shared_ptr<Request>> requests;
};

#endif
