#include "session.h"
#include "item.h"
#include "memcached.h"

#include <boost/algorithm/string.hpp>

#include <map>

void Session::onMessage(const muduo::net::TcpConnectionPtr& conn, 
        muduo::net::Buffer* buffer, muduo::Timestamp time) {
    while(buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string request(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf + 2);

        if(currentCommand != "") {
            if(currentCommand == "add") {
                if(memServer->exists(currentKey)) {
                    conn->send(notStored);
                }
                else {
                    memServer->set(currentKey, request, flags, exptime);
                    conn->send(stored);
                }
            }
            else if(currentCommand == "set") {
                memServer->set(currentKey, request, flags, exptime);
                conn->send(stored);
            }
            else if(currentCommand == "replace") {
                if(memServer->exists(currentKey)) {
                    memServer->set(currentKey, request, flags, exptime);
                    conn->send(stored);
                }
                else {
                    conn->send(notStored);
                }
            }
            else if(currentCommand == "append") {
                if(memServer->exists(currentKey)) {
                    memServer->append(currentKey, request);
                    conn->send(stored);
                }
                else {
                    conn->send(notStored);
                }
                    
            }
            else if(currentCommand == "prepend") {
               if(memServer->exists(currentKey)) {
                   memServer->prepend(currentKey, request);
                   conn->send(stored);
               }
               else {
                   conn->send(notStored);
               }
            }
            else if(currentCommand == "cas") {
                if(memServer->exists(currentKey)) {
                    std::shared_ptr<Item> item = memServer->get(currentKey);
                    uint64_t oldCas = item->getCas();
                    if(oldCas != cas) {
                        conn->send(exists);
                    }
                    else {
                        memServer->set(currentKey, request, flags, exptime);
                        conn->send(stored);
                    }
                }
                else {
                    conn->send(notFound);
                }
            }
            else {
                conn->send(nonExistentCommand);
            }
            currentCommand = "";
            currentKey = "";
        }
        else {
            std::vector<std::string> tokens; 
            boost::split(tokens, request, boost::is_any_of(" "));
            if(tokens[0] == "add") {
                if(validateStorageCommand(tokens, 5, conn)) {
                    setStorageCommandInfo(tokens);
                }
            }
            else if(tokens[0] == "set") {
                if(validateStorageCommand(tokens, 5, conn)) {
                    setStorageCommandInfo(tokens);
                }
            }
            else if(tokens[0] == "replace") {
                if(validateStorageCommand(tokens, 5, conn)) {
                    setStorageCommandInfo(tokens);
                }
            }
            else if(tokens[0] == "append") {
                if(validateStorageCommand(tokens, 5, conn)) {
                    setStorageCommandInfo(tokens);
                }
            }
            else if(tokens[0] == "prepend") {
                if(validateStorageCommand(tokens, 5, conn)) {
                    setStorageCommandInfo(tokens);
                }
            }
            else if(tokens[0] == "cas") {
                if(validateStorageCommand(tokens, 6, conn)) {
                    setStorageCommandInfo(tokens);
                }
            }
            else if(tokens[0] == "get") {
                if(tokens.size() <= 1) {
                    conn->send(nonExistentCommand);
                }
                else {
                    std::vector<std::string> keys(++tokens.begin(), tokens.end());
                    std::map<std::string, std::shared_ptr<Item>> values = memServer->get(keys);    
                    auto iter = ++tokens.begin();
                    while(iter != tokens.end()) {
                        auto itemIter = values.find(*iter);
                        if(itemIter != values.end()) {
                            std::string key = *iter;
                            std::string value = itemIter->second->get();
                            uint16_t flags = itemIter->second->getFlags();
                            size_t size = value.size(); 
                            std::string line = "VALUE " + key + " " + std::to_string(flags) 
                                + " " + std::to_string(size) + "\r\n";
                            conn->send(line);
                            conn->send(value + "\r\n");
                        }
                        ++iter;
                    }
                    conn->send(end);
                }
            }
            else if(tokens[0] == "gets") {
                if(tokens.size() <= 1) {
                    conn->send(nonExistentCommand);
                }
                else {
                    std::vector<std::string> keys(++tokens.begin(), tokens.end());
                    std::map<std::string, std::shared_ptr<Item>> items = memServer->get(keys);
                    auto iter = ++tokens.begin();
                    while(iter != tokens.end()) {
                        auto itemIter = items.find(*iter);
                        if(itemIter != items.end()) {
                            std::string key = *iter;
                            std::string value = itemIter->second->get();
                            uint16_t flags = itemIter->second->getFlags();
                            uint64_t cas = itemIter->second->getCas();
                            size_t size = value.size();

                            std::string line = "VALUE " + key + " " + std::to_string(flags)
                                + " " + std::to_string(exptime) + " " + std::to_string(size)
                                + " " + std::to_string(cas) + "\r\n";
                            conn->send(line);
                            conn->send(value+"\r\n");
                        }
                        ++iter;
                    }
                    conn->send(end);
                }
            }
            else if(tokens[0] == "delete") {
                if(tokens.size() != 2) {
                    conn->send(nonExistentCommand);
                }
                else {
                    if(!memServer->exists(tokens[1])) {
                        conn->send(notFound);
                    }
                    else {
                        memServer->deleteKey(tokens[1]);
                        conn->send(deleted);
                    }
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
    while(begin != end) {
        if(!(isNumber(*begin))) {
            return false;
        }
        ++begin;
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
        result = isAllNumber(tokens.begin()+2, tokens.end());
        if(!result) {
            conn->send(badFormat);
        }
    }

    return result;
}

void Session::setStorageCommandInfo(const std::vector<std::string>& tokens) {
    currentCommand = tokens[0];
    currentKey = tokens[1];
    flags = stoul(tokens[2]);
    exptime = stoul(tokens[3]);
    bytesToRead = stoul(tokens[4]);
    if(tokens.size() == 6) {
        cas = stoull(tokens[5]);
    }
}
