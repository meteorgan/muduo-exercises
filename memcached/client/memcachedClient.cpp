#include "memcachedClient.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstring>

#include <boost/algorithm/string.hpp>

#include <stdexcept>
#include <sstream>

void MemcachedClient::connect() {
    fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        throw std::runtime_error("create socket error: " + std::string(std::strerror(errno)));
    }
    
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    inet_aton(serverIP.c_str(), &serveraddr.sin_addr);
    if(::connect(fd, reinterpret_cast<const struct sockaddr*>(&serveraddr), sizeof(serveraddr)) == -1) {
        char error[64];
        snprintf(error, 64, "connect to server %s:%d error: %s", serverIP.c_str(), port, std::strerror(errno));
        throw std::runtime_error(error);
    }
}

void MemcachedClient::sendRequest(const std::string data) {
    if(::send(fd, data.c_str(), data.length(), 0) == -1) {
        std::string err = "send " + data + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }
}

bool MemcachedClient::sendRequest(const std::string& command, std::string expectResponse, std::string failReponse) {
    sendRequest(command);
    std::string response = readLine(20);
    if(response == expectResponse) {
        return true;
    }
    else if(response == failReponse) {
        return false;
    }
    else {
        throw std::runtime_error(response);
    }
}

std::string MemcachedClient::readLine(size_t maxlen) {
    char* buffer = new char[maxlen+1];
    size_t total  = 0;
    char ch;
    while(total < 2 || buffer[total-2] != '\r' || buffer[total-1] != '\n') {
        ssize_t rn = ::read(fd, &ch, 1);
        if(rn == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
        else if(rn == 0) {
            throw std::runtime_error("server closed");
        }

        buffer[total] = ch;
        total += rn;
    }
    buffer[total-2] = '\0';
    std::string response(buffer);
    delete[] buffer;

    return response;
}

std::string MemcachedClient::readBytes(size_t size) {
    size_t total = size + 2;
    char* buffer = new char[total+1];
    while(total > 0) {
        ssize_t rn = ::read(fd, buffer, total);
        if(rn == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
        else if(rn == 0) {
            throw std::runtime_error("server closed");
        }

        total -= rn;
    }
    buffer[size] = '\0';
    std::string response(buffer);

    delete[] buffer;

    return response;
}

bool MemcachedClient::sendStorageCommand(std::string command, std::string key, std::string value,
        uint32_t expire) {
    size_t valueSize = value.length();
    std::stringstream fmt;
    fmt << command << " " << key << " " << 0 << " " << expire << " " << valueSize << "\r\n";
    sendRequest(fmt.str());

    value += "\r\n";
    return sendRequest(value, "STORED", "NOT_STORED");
}

bool MemcachedClient::set(std::string key, std::string value, uint32_t expire) {
    return sendStorageCommand("set", key, value, expire);
}

bool MemcachedClient::add(std::string key, std::string value, uint32_t expire) {
    return sendStorageCommand("add", key, value, expire);
}

bool MemcachedClient::replace(std::string key, std::string value, uint32_t expire) {
    return sendStorageCommand("replace", key, value, expire);
}

bool MemcachedClient::append(std::string key, std::string value) {
    return sendStorageCommand("append", key, value, 0);
}

bool MemcachedClient::prepend(std::string key, std::string value) {
    return sendStorageCommand("prepend", key, value, 0);
}

bool MemcachedClient::cas(std::string key, std::string value, unsigned long casUnique, uint32_t expire) {
    size_t valueSize = value.size();
    std::stringstream fmt;
    fmt << "cas " << key << " " << 0 << " " << expire << " " << valueSize << " " << casUnique << "\r\n";
    sendRequest(fmt.str());

    value += "\r\n";
    sendRequest(value);
    std::string response = readLine(20);
    if(response == "EXISTS") {
        return false;
    }
    else if(response == "STORED") {
        return true;
    }
    else {
        throw std::runtime_error(response);
    }
}

std::string MemcachedClient::get(std::string key) {
    std::string command = "get " + key + "\r\n";
    sendRequest(command);
    size_t size = 5 + key.length() + 32 + 10 + 32 + 2 + 4;
    std::string response = readLine(size);
    if(response.find("VALUE") == 0) {
        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        size_t bytes = std::stoul(tokens[3]);
        std::string value = readBytes(bytes);
        return value;
    }
    else if(response == "END") {
        throw std::runtime_error("not exist");
    }
    else {
        throw std::runtime_error(response);
    }
}

unsigned long MemcachedClient::getLong(std::string key) {
    std::string value = get(key);
    return std::stoul(value);
}

std::map<std::string, std::string> MemcachedClient::getMulti(std::vector<std::string>& keys)  {
    std::string command("get");
    for(auto& key : keys) {
        command += " " + key;
    }
    command += "\r\n";
    sendRequest(command);

    std::string maxSizeKey = *std::max_element(keys.begin(), keys.end(), 
            [](std::string& key1, std::string& key2) { return key1.length() <= key2.length(); });
    std::map<std::string, std::string> kvs;
    size_t size = 5 + maxSizeKey.length() + 32 + 10 + 32 + 2 + 4;
    while(true) {
        std::string response = readLine(size);
        if(response.find("VALUE") == 0) {
            std::vector<std::string> tokens;
            boost::split(tokens, response, boost::is_any_of(" "));
            std::string key(tokens[1]);
            size_t size = std::stoul(tokens[3]);
            std::string value = readBytes(size);
            kvs[key] = value;
        }
        else if(response == "END") {
            break;
        }
        else {
            throw std::runtime_error(response);
        }
    }

    return kvs;
}

std::pair<std::string, unsigned long> MemcachedClient::gets(std::string key) {
    std::string command = "gets " + key + "\r\n";
    sendRequest(command);
    size_t size = 5 + key.length() + 32 + 10 + 32 + 2 + 4;
    std::string response = readLine(size);
    if(response.find("VALUE") == 0) {
        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        unsigned long casUnique = std::stoul(tokens[4]);
        size_t bytes = std::stoul(tokens[3]);
        std::string value = readBytes(bytes);

        return std::make_pair(value, casUnique);
    }
    else if(response == "END") {
        throw std::runtime_error("not exist");
    }
    else {
        throw std::runtime_error(response);
    }
}

std::pair<unsigned long, unsigned long> MemcachedClient::getsLong(std::string key) {
    std::pair<std::string, unsigned long> res = gets(key);
    unsigned long v = std::stoul(res.first);
    return std::make_pair(v, res.second);
}

std::map<std::string, std::pair<std::string, unsigned long>> MemcachedClient::getsMulti(std::vector<std::string>& keys) {
    std::map<std::string, std::pair<std::string, unsigned long>> kvs;

    std::string command = "gets";
    for(auto& key : keys) {
        command += " " + key;
    }
    command += "\r\n";
    sendRequest(command);

    std::string maxSizeKey = *std::max_element(keys.begin(), keys.end(), 
            [](std::string& k1, std::string& k2) { return k1.length() <= k2.length(); });
    size_t maxlen = 5 + maxSizeKey.length() + 32 + 10 + 32 + 2 + 4;
    while(true) {
        std::string response = readLine(maxlen);
        std::vector<std::string> tokens;
        boost::split(tokens, response, boost::is_any_of(" "));
        if(tokens[0] == "VALUE") {
            std::string key = tokens[1];
            size_t bytes = std::stoul(tokens[3]);
            unsigned long casUnique = std::stoul(tokens[4]);
            std::string value = readBytes(bytes);
            kvs[key] = std::make_pair(value, casUnique);
        }
        else if(tokens[0] == "END") {
            break;
        }
        else {
            throw std::runtime_error(response);
        }
    }

    return kvs;
}

bool MemcachedClient::deleteKey(std::string key) {
    std::string command = "delete " + key + "\r\n";

    return sendRequest(command, "DELETED", "NOT_FOUND");
}

uint64_t MemcachedClient::incr(std::string key, uint64_t value) {
    std::stringstream fmt;
    fmt << "incr " << key << " " << value << "\r\n";
    sendRequest(fmt.str()); 
    
    std::string response = readLine(30);
    if(response == "NOT_FOUND") {
        throw std::runtime_error("NOT_FOUND");
    }
    else if(response.find("ERROR") != response.npos) {
        throw std::runtime_error(response);
    }
    else {
        return std::stoull(response);
    }
}

uint64_t MemcachedClient::decr(std::string key, uint64_t value) {
    std::stringstream fmt;
    fmt << "decr " << key << " " << value << "\r\n";

    std::string response = readLine(30);
    if(response == "NOT_FOUND") {
        throw std::runtime_error("NOT_FOUND");
    }
    else if(response.find("ERROR") != response.npos) {
        throw std::runtime_error(response);
    }
    else {
        return std::stoull(response);
    }
}

bool MemcachedClient::touch(std::string key, uint32_t expire) {
    std::stringstream fmt;
    fmt << "touch " << key << " " << expire << "\r\n";

    return sendRequest(fmt.str(), "TOUCHED", "NOT_FOUND");
}


int main(int args, char** argv) {
    std::string serverIP("127.0.0.1");
    uint16_t port = 11211;

    MemcachedClient client(serverIP, port);
    client.connect();

    client.set("key10", "value10");
    client.add("key4", "value4");
    client.replace("key4", "replace");

    return 0;
}
