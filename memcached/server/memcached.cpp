#include "memcached.h"
#include "session.h"

#include "muduo/net/EventLoop.h"
#include "muduo/base/Logging.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

Memcached::Memcached(muduo::net::EventLoop* loop, 
        const muduo::net::InetAddress& listenAddr) 
    : listenAddr(listenAddr), casUnique(0), server(loop, listenAddr, "Memcached"),
      inspectorLoop(), inspector(inspectorLoop.startLoop(), muduo::net::InetAddress(11215), "memcached-stats") {
        server.setConnectionCallback(boost::bind(&Memcached::onConnection, this, _1));

        inspector.add("memcached", "stats", boost::bind(&MemcachedStat::report, &stat), "statistics of memcached");
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

        ++stat.currConnections;
        ++stat.totalConnections;
    }
    else {
        LOG_INFO << conn->peerAddress().toIpPort() << " is DOWN.";
        sessions.erase(name);
        --stat.currConnections;
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

        stat.totalItems++;
        stat.currItems++;
    }
}

void Memcached::append(const std::string& key, const std::string& app) {
    ++casUnique;
    auto itemPtr = (*items.find(key)).second;
    itemPtr->append(app, casUnique);
}

void Memcached::prepend(const std::string& key, const std::string& pre) {
    ++casUnique;
    auto itemPtr = (*items.find(key)).second;
    itemPtr->prepend(pre, casUnique);
}

std::shared_ptr<Item> Memcached::get(const std::string& key) {
    auto iter = items.find(key);
    assert(iter != items.end());
    return (*iter).second;
}

std::map<std::string, std::shared_ptr<Item>> Memcached::get(const std::vector<std::string>& keys) {
    std::map<std::string, std::shared_ptr<Item>> results;
    for(auto key : keys) {
        auto iter = items.find(key);
        if(iter != items.end()) {
            if(iter->second->isExpire()) {
                items.erase(iter);
                stat.currItems--;
            }
            else {
                results[key] = iter->second;
            }
        }
    }

    return results;
}

void Memcached::deleteKey(const std::string& key) {
    items.erase(key);
    stat.currItems--;
}

uint64_t Memcached::incr(const std::string& key, uint64_t increment) {
    ++casUnique;
    auto iter = items.find(key);
    assert(iter != items.end());
    uint64_t result = iter->second->incr(increment, casUnique);

    return result;
}

uint64_t Memcached::decr(const std::string& key, uint64_t decrement) {
    ++casUnique;
    auto iter = items.find(key);
    assert(iter != items.end());
    uint64_t result = iter->second->decr(decrement, casUnique);

    return result;
}

void Memcached::touch(const std::string& key, uint32_t exptime) {
    auto iter = items.find(key);
    assert(iter != items.end());
    iter->second->touch(exptime);
}

void Memcached::flush_all(uint32_t exptime) {
    uint32_t t = static_cast<uint32_t>(muduo::Timestamp::now().secondsSinceEpoch()) - 1;
    if(exptime != 0) {
        t = exptime;
    }
    for(auto& item : items) {
        item.second->touch(t);
    }
}

bool Memcached::exists(const std::string& key) {
    auto iter = items.find(key);
    if(iter == items.end()) {
        return false;
    }
    else {
        bool expired = iter->second->isExpire();
        if(expired) {
            items.erase(iter);
            stat.currItems--;
        }
        return !expired;
    }
}

MemcachedStat& Memcached::memStats() {
    return stat;    
}



int main(int argc, char** argv) {
    std::string defaultIP = "127.0.0.1";
    uint16_t defaultPort = 11211;
    if(argc >= 3) {
        defaultIP = std::string(argv[1]);
        defaultPort = static_cast<uint16_t>(atoi(argv[2]));
    }
    
    muduo::net::InetAddress listenAddr(defaultIP, defaultPort);
    muduo::net::EventLoop loop;
    Memcached server(&loop, listenAddr);    
    server.start();

    loop.loop();

    return 0;
}
