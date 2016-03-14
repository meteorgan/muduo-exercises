#include "memcached.h"

#include "muduo/base/Logging.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

Memcached::Memcached(muduo::net::EventLoop* loop, 
        const muduo::net::InetAddress& listenAddr) 
    : listenAddr(listenAddr), casUnique(0), server(loop, listenAddr, "Memcached") {
        server.setConnectionCallback(boost::bind(&Memcached::onConnection, this, _1));
}

void Memcached::start() {
    server.start();
}

void Memcached::onConnection(const muduo::net::TcpConnectionPtr& conn) {
    LOG_INFO << conn->peerAddress().toIpPort() << " is " << (conn->connected() ? "UP" : "DOWN");
}

void Memcached::onMessage(const muduo::net::TcpConnectionPtr& conn, 
        muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string request(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf + 2);

        if(currentCommand != "") {
        }
        else {
            std::vector<std::string> tokens; 
            boost::split(tokens, request, boost::is_any_of(" "));
            if(tokens[0] == "add") {
                validateStorageCommand(tokens, 5, conn);
            }
            else if(tokens[0] == "set") {
                validateStorageCommand(tokens, 5, conn);
            }
            else if(tokens[0] == "replace") {
                validateStorageCommand(tokens, 5, conn);
            }
            else if(tokens[0] == "append") {
                validateStorageCommand(tokens, 5, conn);
            }
            else if(tokens[0] == "prepend") {
                validateStorageCommand(tokens, 5, conn);
            }
            else if(tokens[0] == "cas") {
                validateStorageCommand(tokens, 6, conn);
            }
            else if(tokens[0] == "get") {
                if(tokens.size() <= 1) {
                    conn->send(nonExistentCommand);
                }
            }
            else if(tokens[0] == "gets") {
                if(tokens.size() <= 1) {
                    conn->send(nonExistentCommand);
                }
            }
            else if(tokens[0] == "delete") {
                if(tokens.size() != 2) {
                    conn->send(nonExistentCommand);
                }
            }
            else if(tokens[0] == "incr") {
                if(tokens.size() != 3) {
                    conn->send(nonExistentCommand);
                }
                else if(!isNumber(tokens[2])) {
                    conn->send(nonNumeric);
                }
            }
            else if(tokens[0] == "decr") {
                if(tokens.size() != 3) {
                    conn->send(nonExistentCommand);
                }
                else if(!isNumber(tokens[2])) {
                    conn->send(nonNumeric);
                }
            }
            else if(tokens[0] == "touch") {
                if(tokens.size() != 3) {
                    conn->send(nonExistentCommand);
                }
                else if(!isNumber(tokens[2])) {
                    conn->send(invalidExptime);
                }
            }
            else if(tokens[0] == "stats") {
                if(tokens.size() != 1) {
                    conn->send(nonExistentCommand);
                }
            }
            else {
                conn->send(nonExistentCommand);
            }
        }
    }
}

bool Memcached::isAllNumber(std::vector<std::string>::const_iterator begin, 
        std::vector<std::string>::const_iterator end) {
    while(begin++ != end) {
        if(!(isNumber(*begin))) {
            return false;
        }
    }

    return true;
}

bool Memcached::isNumber(const std::string& str) {
    for(auto& ch : str) {
        if(!(isdigit(ch))) {
            return false;
        }
    }

    return true;
}

bool Memcached::validateStorageCommand(const std::vector<std::string>& tokens, size_t size, 
        const muduo::net::TcpConnectionPtr& conn) {
    bool result = true;
    if(tokens.size() != size) {
        conn->send(nonExistentCommand);
        result = false;
    }
    else {
        result = isAllNumber(++tokens.begin(), tokens.end());
        if(!result) {
            conn->send(badFormat);
        }
    }

    return result;
}


int main(int argc, char** argv) {

    return 0;
}
