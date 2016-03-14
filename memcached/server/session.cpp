#include "session.h"

#include <boost/algorithm/string.hpp>


void Session::onMessage(const muduo::net::TcpConnectionPtr& conn, 
        muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string request(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf + 2);

        if(currentCommand != "") {
            if(currentCommand == "add") {
            }
            else if(currentCommand == "set") {
            }
            else if(currentCommand == "replace") {
            }
            else if(currentCommand == "append") {
            }
            else if(currentCommand == "prepend") {
            }
            else if(currentCommand == "cas") {
            }
            else {
            }
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

bool Session::isAllNumber(std::vector<std::string>::const_iterator begin, 
        std::vector<std::string>::const_iterator end) {
    while(begin++ != end) {
        if(!(isNumber(*begin))) {
            return false;
        }
    }

    return true;
}

bool Session::isNumber(const std::string& str) {
    for(auto& ch : str) {
        if(!(isdigit(ch))) {
            return false;
        }
    }

    return true;
}

bool Session::validateStorageCommand(const std::vector<std::string>& tokens, size_t size, 
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
