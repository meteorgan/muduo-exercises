#include "dataServer.h"
#include "genNumberExecutor.h"
#include "averageExecutor.h"
#include "sortExecutor.h"
#include "freqExecutor.h"

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
    muduo::net::EventLoop* workerLoop = workerLoopThread.startLoop();
    for(auto& addr : workerAddrs) {
        std::string workerId = "worker-" + std::to_string(seq);
        auto worker = std::unique_ptr<muduo::net::TcpClient>(
                new muduo::net::TcpClient(workerLoop, addr, workerId.c_str()));
        worker->setConnectionCallback(boost::bind(&DataServer::onWorkerConnection, this, _1));
        worker->setMessageCallback(boost::bind(&DataServer::onWorkerMessage, this, _1, _2, _3));
        worker->connect();
        std::string peer(addr.toIpPort().c_str());
        workers.insert(std::make_pair(peer, std::move(worker)));

        ++seq;
    }

    {
        size = workers.size();
        std::unique_lock<std::mutex> lock(mt);
        while(size > 0)
            cond.wait(lock);
    }
    LOG_INFO << "server connected to all workers";
}

void DataServer::onClientConnection(const muduo::net::TcpConnectionPtr& conn) {
    LOG_INFO << conn->peerAddress().toIpPort() << (conn->connected() ? " is UP" : " is DOWN");
}

// command:
// 1. genNumber n   response: ok
// 2. average       response: number<double>
// 3. median        response: number<int64_t>
// 4. sort          response: ok
// 5. freq n        response: n1 n2 ... n
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
            dataExecutor = &executor;
            int64_t number = std::stol(tokens[1]);
            char mode = tokens[2][0];
            executor.execute(number, mode);

            conn->send("OK\r\n");
        }
        else if(command == "average") {
            AverageExecutor executor(connections);
            dataExecutor = &executor;
            double average = executor.execute();

            conn->send(std::to_string(average) + "\r\n");
        }
        else if(command == "median") {
        }
        else if(command == "sort") {
            SortExecutor executor(connections);
            dataExecutor = &executor;
            executor.execute();

            conn->send("OK\r\n");
        }
        else if(command.find("freq") == 0) {
            size_t freqNumber = std::stoi(tokens[1]);
            FreqExecutor executor(connections);
            dataExecutor = &executor;
            std::vector<std::pair<int64_t, int64_t>> freqs = executor.execute(freqNumber);
            std::string response = "";
            for(auto& freq : freqs) {
                response += " " + std::to_string(freq.first) + " " + std::to_string(freq.second);
            }
            response += "\r\n";
            conn->send(response);
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

void DataServer::onWorkerMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf, muduo::Timestamp time) {
    dataExecutor->onMessage(conn, buf, time);
}


int main(int argc, char** argv) {
    if(argc <= 1) {
        LOG_ERROR << "usage: DataServer [-h ip port] worker1IP worker1Port ...";
        return -1;
    }

    std::string serverIp = "127.0.0.1";
    uint16_t port = 9980;
    int pos = 1;
    if(strcmp(argv[1], "-h") == 0) {
        serverIp = std::string(argv[1]);
        port = static_cast<uint16_t>(std::stoi(argv[2]));

        pos += 2;
    }

    if(argc <= pos + 1) {
        LOG_ERROR << "usage: DataServer [-h ip port] worker1IP worker1Port ...";
        return -1;
    }
    std::vector<muduo::net::InetAddress> workerAddrs;
    for(int i = pos; i < argc; i += 2) {
        std::string ip = std::string(argv[i]);
        uint16_t p = static_cast<uint16_t>(std::stoi(argv[i+1]));
        muduo::net::InetAddress addr(ip, p);
        workerAddrs.push_back(addr);
    }

    muduo::net::InetAddress addr(serverIp, port);
    muduo::net::EventLoop loop;
    DataServer server(&loop, addr, workerAddrs);
    server.start();

    loop.loop();

    return 0;
}
