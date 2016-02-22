#include "averageExecutor.h"

#include <boost/algorithm/string.hpp>

double AverageExecutor::execute() {
    sum = 0;
    number = 0;
    size = connections.size();

    {
        std::unique_lock<std::mutex> lock(mt);
        while(size > 0)
            cond.wait(lock);
    }

    return (double)sum/ number;
}

void AverageExecutor::onMessge(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf, muduo::Timestamp time) {
    while(buf->findCRLF()) {
        const char* crlf = buf->findCRLF();
        std::string response(buf->peek(), crlf);
        buf->retrieveUntil(crlf + 2);

        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        if(tokens[0] == "average") {
            sum += std::stol(tokens[1]);
            number += std::stol(tokens[2]);

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
