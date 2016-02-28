#include "averageExecutor.h"

#include <boost/algorithm/string.hpp>

double AverageExecutor::execute() {
    sum = 0;
    number = 0;
    std::string message = "average\r\n";
    for(auto& conn : connections)
        conn.second->send(message);

    size = connections.size();
    {
        std::unique_lock<std::mutex> lock(mt);
        while(size > 0)
            cond.wait(lock);
    }

    return static_cast<double>(sum) / static_cast<double>(number);
}

void AverageExecutor::onMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf, muduo::Timestamp time) {
    while(buf->findCRLF()) {
        const char* crlf = buf->findCRLF();
        std::string response(buf->peek(), crlf);
        buf->retrieveUntil(crlf + 2);

        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        if(tokens[0] == "average") {
            number += std::stol(tokens[1]);
            sum += std::stol(tokens[2]);
            LOG_INFO << "receive average " << response << " from " << conn->peerAddress().toIpPort();

            {
                std::unique_lock<std::mutex> lock(mt);
                size -= 1;
                cond.notify_all();
            }
        }
        else {
            LOG_ERROR << conn->peerAddress().toIpPort() << " response error: " << response;
        }
    }
}
