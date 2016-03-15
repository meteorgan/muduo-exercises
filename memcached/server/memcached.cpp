#include "memcached.h"

#include "muduo/net/EventLoop.h"
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
    std::string name(conn->name().c_str());
    if(conn->connected()) {
        LOG_INFO << conn->peerAddress().toIpPort() << " is UP.";
        std::unique_ptr<Session> session(new Session(this, conn));
        sessions[name] = std::move(session);
    }
    else {
        LOG_INFO << conn->peerAddress().toIpPort() << " is DOWN.";
        sessions.erase(name);
    }
}

void Memcached::set(const std::string& key, const std::string& value, 
        uint16_t flags, uint32_t exptime) {
    ++casUnique;

    auto iter = items.find(key);
    if(iter != items.end()) {
        iter->second->set(value, flags, exptime, casUnique);
    }
    else {
        std::shared_ptr<Item> item(new Item(key, value, flags, exptime, casUnique));
        items[key] = item;
    }
}

std::map<std::string, std::shared_ptr<Item>> Memcached::get(std::vector<std::string>& keys) {
    std::map<std::string, std::shared_ptr<Item>> results;
    for(auto key : keys) {
        auto iter = items.find(key);
        if(iter != items.end()) {
            results[key] = iter->second;
        }
    }

    return results;
}

bool Memcached::exists(const std::string& key) {
    return (items.find(key) != items.end());
}



int main(int argc, char** argv) {
    std::string defaultIP = "127.0.0.1";
    uint16_t defaultPort = 11211;
    if(argc >= 3) {
        defaultIP = std::string(argv[1]);
        defaultPort = atoi(argv[2]);
    }
    
    muduo::net::InetAddress listenAddr(defaultIP, defaultPort);
    muduo::net::EventLoop loop;
    Memcached server(&loop, listenAddr);    
    server.start();

    loop.loop();

    return 0;
}
