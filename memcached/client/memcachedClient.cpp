#include "memcachedClient.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>

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

void MemcachedClient::sendRequest(std::string data) {
    if(::send(fd, data.c_str(), data.length(), 0) == -1) {
        std::string err = "send " + data + std::string(std::strerror(errno));
        throw std::runtime_error(err);
    }
}

std::string MemcachedClient::readLine(size_t size) {
    char* buffer = new char[size+1];
    size_t total  = 0;
    while(total < 2 || buffer[total-1] != '\n' || buffer[total-2] != '\r') {
        ssize_t rn = ::read(fd, buffer, size);
        if(rn == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
        total += rn;
    }
    buffer[total-2] = '\0';
    std::string response(buffer);
    delete[] buffer;

    return response;
}

void MemcachedClient::sendCommand(std::string command, std::string key, std::string value,
        uint32_t expire) {
    size_t valueSize = value.length();
    std::stringstream fmt;
    fmt << command << " " << key << " " << 0 << " " << expire << " " << valueSize << "\r\n";
    sendRequest(fmt.str());

    value += "\r\n";
    sendRequest(value);

    std::string response = readLine(20);
    if(response != "STORED") {
        throw std::runtime_error(response);
    }
}

void MemcachedClient::set(std::string key, std::string value, uint32_t expire) {
    sendCommand("set", key, value, expire);
}

void MemcachedClient::add(std::string key, std::string value, uint32_t expire) {
    sendCommand("add", key, value, expire);
}

void MemcachedClient::replace(std::string key, std::string value, uint32_t expire) {
    sendCommand("replace", key, value, expire);
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
