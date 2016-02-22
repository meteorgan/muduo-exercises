#include "dataServer.h"
#include "genNumberExecutor.h"

#include "muduo/base/Logging.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

DataServer::DataServer(muduo::net::EventLoop* loop,
        muduo::net::InetAddress& listenAddr,
        std::vector<muduo::net::InetAddress>& addrs)
    :loop_(loop), server_(loop, listenAddr, "DataServer"), 
    workerAddrs(addrs), size(0) {
    server_.setConnectionCallback(boost::bind(&DataServer::onClientConnection, this, _1));
    server_.setMessageCallback(boost::bind(&DataServer::onClientMessage, this, _1, _2, _3));
}

void DataServer::start() {
    server_.start();
    int seq = 1;
    for(auto& addr : workerAddrs) {
        std::string workerId = "worker-" + std::to_string(seq);
        auto worker = std::unique_ptr<muduo::net::TcpClient>(
                new muduo::net::TcpClient(loop_, addr, workerId.c_str()));
        worker->setConnectionCallback(boost::bind(&DataServer::onWorkerConnection, this, _1));
        worker->connect();
        std::string peer(worker->connection()->peerAddress().toIpPort().c_str());
        workers.insert(std::make_pair(peer, std::move(worker)));

        ++seq;
    }

    {
        size = workers.size();
        std::unique_lock<std::mutex> lock;
        while(size > 0)
            cond.wait(lock);
    }
}

void DataServer::onClientConnection(const muduo::net::TcpConnectionPtr& conn) {
    LOG_INFO << conn->peerAddress().toIpPort() << (conn->connected() ? " is UP" : " is DOWN");
}

// command:
// 1. genNumber n   response: ok
// 2. average       response: number
// 3. freq n        response: n1 n2 ... n
// 4. sort          response: ok
void DataServer::onClientMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf, muduo::Timestamp time) {
    while(buf->findCRLF()) {
        const char* crlf = buf->findCRLF();
        std::string command(buf->peek(), crlf);
        buf->retrieveUntil(crlf+2);
        std::vector<std::string> tokens;
        boost::split(tokens, command, boost::is_any_of(" "));
        if(command.find("genNumber") == 0) {
            GenNumberExecutor executor(connections);        
            for(auto& worker : workers) 
                worker.second->setMessageCallback(boost::bind(&GenNumberExecutor::onMessage, &executor, _1, _2, _3));
            int64_t number = std::stol(tokens[1]);
            char mode = tokens[2][0];
            executor.execute(number, mode);
        }
        else if(command == "average") {
        }
        else if(command == "sort") {
        }
        else if(command.find("freq") == 0) {
        }
        else {
            LOG_ERROR << "receive unknown command [" << command << "] from " 
                      << conn->peerAddress().toIpPort();
            conn->shutdown();
        }
    }
}

void DataServer::onWorkerConnection(const muduo::net::TcpConnectionPtr& conn) {
    std::string peer(conn->peerAddress().toIpPort().c_str());
    if(conn->connected()) {
        LOG_INFO << "connected to worker: " << peer;
        connections.insert(std::make_pair(peer, conn));

        {
            std::unique_lock<std::mutex> lock(mt);
            size -= 1;
            cond.notify_all();
        }
    }
    else {
        LOG_INFO << "worker: " << peer << " is down";
        workers.erase(peer);
        connections.erase(peer);
    }
}
