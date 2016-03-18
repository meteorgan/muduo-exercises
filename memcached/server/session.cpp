#include "session.h"
#include "item.h"
#include "memcached.h"

#include <boost/algorithm/string.hpp>

#include <map>

void Session::onMessage(const muduo::net::TcpConnectionPtr& conn, 
        muduo::net::Buffer* buffer, muduo::Timestamp time) {

    // read command
    while(currentCommand == "" && buffer->findCRLF()) {
        const char* crlf = buffer->findCRLF();
        std::string request(buffer->peek(), crlf);
        buffer->retrieveUntil(crlf + 2);

        handleCommand(conn, request);
    }

    // read data chunk 
    if(currentCommand != "" && buffer->readableBytes() >= bytesToRead) {
        std::string request(buffer->peek(), bytesToRead);        
        buffer->retrieve(bytesToRead);

        handleDataChunk(conn, request);
        currentCommand = "";
        currentKey = "";
    }
}

void Session::handleDataChunk(const muduo::net::TcpConnectionPtr& conn,
        const std::string& request) {
    if(currentCommand == "add") {
        std::string result;
        if(memServer->exists(currentKey)) {
            result = notStored;
        }
        else {
            memServer->set(currentKey, request, flags, exptime);
            result = stored;
        }
        if(!noreply) {
            conn->send(result);
        }
    }
    else if(currentCommand == "set") {
        memServer->set(currentKey, request, flags, exptime);
        if(!noreply) {
            conn->send(stored);
        }
    }
    else if(currentCommand == "replace") {
        std::string result;
        if(memServer->exists(currentKey)) {
            memServer->set(currentKey, request, flags, exptime);
            result = stored;
        }
        else {
            result = notStored;
        }
        if(!noreply) {
            conn->send(result);
        }
    }
    else if(currentCommand == "append") {
        std::string result;
        if(memServer->exists(currentKey)) {
            memServer->append(currentKey, request);
            result = stored;
        }
        else {
            result = notStored;
        }
        if(!noreply) {
            conn->send(result);
        }
    }
    else if(currentCommand == "prepend") {
        std::string result;
       if(memServer->exists(currentKey)) {
           memServer->prepend(currentKey, request);
           result = stored;
       }
       else {
           result = notStored;
       }
       if(!noreply) {
           conn->send(result);
       }
    }
    else if(currentCommand == "cas") {
        std::string result;
        if(memServer->exists(currentKey)) {
            std::shared_ptr<Item> item = memServer->get(currentKey);
            uint64_t oldCas = item->getCas();
            if(oldCas != cas) {
                result = exists;
            }
            else {
                memServer->set(currentKey, request, flags, exptime);
                result = stored;
            }
        }
        else {
            result = notFound;
        }
        if(!noreply) {
            conn->send(result);
        }
    }
    else {
        conn->send(nonExistentCommand);
    }
}

void Session::handleCommand(const muduo::net::TcpConnectionPtr& conn,
        const std::string& request) {
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
        handleGet(conn, tokens);
    }
    else if(tokens[0] == "gets") {
        handleGetMulti(conn, tokens);
    }
    else if(tokens[0] == "delete") {
        handleDelete(conn, tokens);
    }
    else if(tokens[0] == "incr") {  //忽略多余的参数
        handleIncr(conn, tokens);
    }
    else if(tokens[0] == "decr") {
        handleDecr(conn, tokens);
    }
    else if(tokens[0] == "touch") {
        handleTouch(conn, tokens);
    }
    else if(tokens[0] == "stats") {
        handleStats(conn, tokens);
    }
    else if(tokens[0] == "flush_all") {
        handleFlushAll(conn, tokens);
    }
    else if(tokens[0] == "quit") {
        conn->shutdown();
    }
    else {
        conn->send(nonExistentCommand);
    }
}

void Session::handleGet(const muduo::net::TcpConnectionPtr& conn, 
        const std::vector<std::string>& tokens) {
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

void Session::handleGetMulti(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
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
                    + " " + std::to_string(size) + " " + std::to_string(cas) + "\r\n";
                conn->send(line);
                conn->send(value+"\r\n");
            }
            ++iter;
        }
            conn->send(end);
    }
}

void Session::handleDelete(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
    noreply = false;
    std::string response("");
    if(tokens.size() < 2) {
        response = nonExistentCommand;
    }
    else if(tokens.size() > 3) {
        response = deleteArgumentError;
    }
    else {
        noreply = tokens.size() == 3 && tokens[2] == NOREPLY;
        if(!memServer->exists(tokens[1])) {
            response = notFound;
        }
        else {
            memServer->deleteKey(tokens[1]);
            response = deleted;
        }
    }
    if(!noreply) {
        conn->send(response);
    }
}

void Session::handleIncr(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
    noreply = false;
    std::string response("");
    if(tokens.size() < 3) {
        response = nonExistentCommand;
    }
    else if(!isUint64(tokens[2])) {
        response = invalidDeltaArgument;
    }
    else if(!memServer->exists(tokens[1])) {
        response = notFound;
    }
    else {
        std::shared_ptr<Item> item = memServer->get(tokens[1]);
        if(!isUint64(item->get())) {
            response = nonNumeric;
        }
        else {
            uint64_t result = memServer->incr(tokens[1], tokens[2]);
            noreply = tokens.size() > 3 && tokens[3] == NOREPLY;
            response= std::to_string(result) + "\r\n";
        }
    }
    if(!noreply) {
        conn->send(response);
    }
}

void Session::handleDecr(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
    noreply = false;
    std::string response("");
    if(tokens.size() < 3) {
        response = nonExistentCommand;
    }
    else if(!isUint64(tokens[2])) {
        response = invalidDeltaArgument;
    }
    else if(!memServer->exists(tokens[1])) {
        response = notFound;
    }
    else {
        std::shared_ptr<Item> item = memServer->get(tokens[1]);
        if(!isUint64(item->get())) {
            response = nonNumeric;
        }
        else {
            uint64_t result = memServer->decr(tokens[1], tokens[2]);
            noreply = tokens.size() > 3 && tokens[3] == NOREPLY;
            response = std::to_string(result) + "\r\n";
        }
    }
    if(!noreply) {
        conn->send(response);
    }
}

void Session::handleTouch(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
    noreply = false;
    std::string response("");
    if(tokens.size() < 3) {
        response = nonExistentCommand;
    }
    else if(!isUint32(tokens[2])) {
        response = invalidExptime;
    }
    else {
        noreply = tokens.size() > 3 && tokens[3] == NOREPLY;

        if(memServer->exists(tokens[1])) {
            memServer->touch(tokens[1], tokens[2]);
            response = touched;
        }
        else {
            response = notFound;
        }
    }
    if(!noreply) {
        conn->send(response);
    }
}

void Session::handleStats(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
    if(tokens.size() != 1) {
        conn->send(nonExistentCommand);
    }
}

void Session::handleFlushAll(const muduo::net::TcpConnectionPtr& conn,
        const std::vector<std::string>& tokens) {
    noreply = false;
    exptime = 0;
    std::string result("");
    if(tokens.size() >= 2) {
        if(isUint32(tokens[1])) {
            exptime = std::stoul(tokens[1]);
            noreply = tokens.size() >= 3 && tokens[2] == NOREPLY;
            memServer->flush_all(exptime);
            result = OK;
        }
        else if(tokens[1] == NOREPLY) {
            noreply = true;
            memServer->flush_all(exptime);
            result = OK;
        }
        else {
           result = nonExistentCommand; 
        }
    }
    else {
        memServer->flush_all(exptime);
        result = OK;
    }

    if(!noreply) {
        conn->send(result);
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
    if(str.size() == 0) {
        return false;
    }
    for(auto& ch : str) {
        if(!(isdigit(ch))) {
            return false;
        }
    }

    return true;
}

bool Session::isUint16(const std::string& str) {
    return isUint(str, maxUint16);
}

bool Session::isUint32(const std::string& str) {
    return isUint(str, maxUint32);
}

bool Session::isUint64(const std::string& str) {
    return isUint(str, maxUint64);
}

bool Session::isUint(const std::string& str, const std::string& uint) {
    return (isNumber(str)
            && (str.length() < uint.length() 
                || (str.length() == uint.length() && str < uint)));
}

bool Session::validateStorageCommand(const std::vector<std::string>& tokens, size_t size, 
        const muduo::net::TcpConnectionPtr& conn) {
    bool result = true;
    if(tokens.size() < size) {
        conn->send(nonExistentCommand);
        result = false;
    }
    else {
        result = isUint16(tokens[2]) && isUint32(tokens[3]) && isUint32(tokens[4]);
        if(size == 6) {
            result = result && isUint64(tokens[5]);
        }
        noreply = tokens.size() > size && tokens[size] == NOREPLY;
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
