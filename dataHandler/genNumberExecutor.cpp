#include "genNumberExecutor.h"

void GenNumberExecutor::execute(int64_t number, char mode) {
    size = connections.size();
    std::string message = "gen_numbers " + std::to_string(number)
        + " " + mode + "\r\n";
    for(auto& conn: connections) {
        conn.second->send(message);
    }
    LOG_INFO << "send message " << message.substr(0, message.size()-2) << " to all workers";

    {
        std::unique_lock<std::mutex> lock(mt);
        while(size > 0)
            cond.wait(lock);
    }
} 

void GenNumberExecutor::onMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf, muduo::Timestamp time) {
    while(buf->findCRLF()) {
        const char* crlf = buf->findCRLF();
        std::string response(buf->peek(), crlf);
        buf->retrieveUntil(crlf + 2);

        if(response == "gen_num") {
            LOG_INFO << conn->peerAddress().toIpPort() << " generate numbers finished";

            {
                std::unique_lock<std::mutex> lock(mt);
                size -= 1;
                cond.notify_all();
            }
        }
        else {
            LOG_ERROR << conn->peerAddress().toIpPort() << " response error " << response;
            conn->shutdown();
        }
    }
}
