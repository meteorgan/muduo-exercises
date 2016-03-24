#include "memcached.h"
#include "session.h"

#include "muduo/net/EventLoop.h"
#include "muduo/base/Logging.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

Memcached::Memcached(muduo::net::EventLoop* loop, 
        const muduo::net::InetAddress& listenAddr, int threadNum) 
    : listenAddr(listenAddr), casUnique(0), numThread(threadNum), server(loop, listenAddr, "Memcached"),
      inspectorLoopThread(), inspector(inspectorLoopThread.startLoop(), muduo::net::InetAddress(11215), "memcached-stats") {
        server.setConnectionCallback(boost::bind(&Memcached::onConnection, this, _1));

        inspector.add("memcached", "stats", boost::bind(&MemcachedStat::report, &stats_), "statistics of memcached");
}

void Memcached::start() {
    server.setThreadNum(numThread);
    server.start();
}

void Memcached::onConnection(const muduo::net::TcpConnectionPtr& conn) {
    std::string name(conn->name().c_str());
    if(conn->connected()) {
        LOG_INFO << name << " is UP.";
        conn->setTcpNoDelay(true);
        std::unique_ptr<Session> session(new Session(this, conn));
        sessions[name] = std::move(session);

        stats_.addCurrConnections(1);
        stats_.addTotalConnections();
    }
    else {
        LOG_INFO << "inputBuffer size: " << conn->inputBuffer()->internalCapacity() 
                 << " outputBuffer size: " << conn->outputBuffer()->internalCapacity();
        sessions.erase(name);
        stats_.addCurrConnections(-1);
    }
}

void Memcached::set(const std::string& key, const std::string& value, 
        uint16_t flags, uint32_t exptime) {
    ++casUnique;

    size_t index = hashFunc(key) % kShards; 
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto iter = shards[index].items.find(key);
        if(iter != shards[index].items.end()) {
            iter->second->set(value, flags, exptime, casUnique);
        }
        else {
            std::shared_ptr<Item> item(new Item(key, value, flags, exptime, casUnique));
            shards[index].items[key] = item;

            stats_.addTotalItems();
            stats_.addCurrItems(1);
        }
    }
}

void Memcached::append(const std::string& key, const std::string& app) {
    ++casUnique;

    size_t index = hashFunc(key) % kShards; 
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto itemPtr = (*shards[index].items.find(key)).second;
        itemPtr->append(app, casUnique);
    }
}

void Memcached::prepend(const std::string& key, const std::string& pre) {
    ++casUnique;

    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto itemPtr = (*shards[index].items.find(key)).second;
        itemPtr->prepend(pre, casUnique);
    }
}

std::shared_ptr<const Item> Memcached::get(const std::string& key) {
    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto iter = shards[index].items.find(key);
        assert(iter != shards[index].items.end());
        return (*iter).second;
    }
}

std::map<std::string, std::shared_ptr<const Item>> Memcached::get(const std::vector<std::string>& keys) {
    std::map<std::string, std::shared_ptr<const Item>> results;
    for(auto key : keys) {
        size_t index = hashFunc(key) % kShards;
        {
            std::lock_guard<std::mutex> lock(shards[index].itemLock);
            auto iter = shards[index].items.find(key);
            if(iter != shards[index].items.end()) {
                if(iter->second->isExpire()) {
                    shards[index].items.erase(iter);
                    stats_.addCurrItems(-1);
                }
                else {
                    results[key] = iter->second;
                }
            }
        }
    }

    return results;
}

void Memcached::deleteKey(const std::string& key) {
    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        shards[index].items.erase(key);
    }
    stats_.addCurrItems(-1);
}

uint64_t Memcached::incr(const std::string& key, uint64_t increment) {
    ++casUnique;
    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto iter = shards[index].items.find(key);
        assert(iter != shards[index].items.end());
        uint64_t result = iter->second->incr(increment, casUnique);

        return result;
    }
}

uint64_t Memcached::decr(const std::string& key, uint64_t decrement) {
    ++casUnique;
    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto iter = shards[index].items.find(key);
        assert(iter != shards[index].items.end());
        uint64_t result = iter->second->decr(decrement, casUnique);
        return result;
    }
}

void Memcached::touch(const std::string& key, uint32_t exptime) {
    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto iter = shards[index].items.find(key);
        assert(iter != shards[index].items.end());
        iter->second->touch(exptime);
    }
}

void Memcached::flush_all(uint32_t exptime) {
    uint32_t t = static_cast<uint32_t>(muduo::Timestamp::now().secondsSinceEpoch()) - 1;
    if(exptime != 0) {
        t = exptime;
    }
    for(int i = 0; i < kShards; ++i) {
        std::lock_guard<std::mutex> lock(shards[i].itemLock);
        for(auto& item : shards[i].items) {
            item.second->touch(t);
        }
    }
}

bool Memcached::exists(const std::string& key) {
    size_t index = hashFunc(key) % kShards;
    {
        std::lock_guard<std::mutex> lock(shards[index].itemLock);
        auto iter = shards[index].items.find(key);
        if(iter == shards[index].items.end()) {
            return false;
        }
        else {
            bool expired = iter->second->isExpire();
            if(expired) {
                shards[index].items.erase(iter);
                stats_.addCurrItems(-1);
            }
            return !expired;
        }
    }
}

MemcachedStat& Memcached::memStats() {
    return stats_;    
}



int main(int argc, char** argv) {
    std::string defaultIP = "127.0.0.1";
    uint16_t defaultPort = 11211;
    if(argc >= 3) {
        defaultIP = std::string(argv[1]);
        defaultPort = static_cast<uint16_t>(atoi(argv[2]));
    }
    int threadNum = 1;
    if(argc >= 4) {
        threadNum = atoi(argv[3]);
    }
    
    muduo::net::InetAddress listenAddr(defaultIP, defaultPort);
    muduo::net::EventLoop loop;
    Memcached server(&loop, listenAddr, threadNum);    
    server.start();

    loop.loop();

    return 0;
}
