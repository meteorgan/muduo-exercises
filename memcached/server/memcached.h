#ifndef MEMCACHED_H
#define MEMCACHED_H

#include "muduo/net/TcpConnection.h"
#include "muduo/net/TcpServer.h"

#include "item.h"

#include <map>

class Session;

class Memcached {
    public:
        Memcached(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr);

        void start();

        void set(const std::string& key, const std::string& value, uint16_t flags, uint32_t exptime);
       
        void append(const std::string& key, const std::string& app);

        void prepend(const std::string& key, const std::string& pre);

        std::shared_ptr<Item> get(const std::string& key);

        std::map<std::string, std::shared_ptr<Item>> get(const std::vector<std::string>& keys);

        void deleteKey(const std::string& key);

        uint64_t incr(const std::string& key, uint64_t value);

        uint64_t decr(const std::string& key, uint64_t value);

        void touch(const std::string& key, uint32_t exptime);

        void stats();

        bool exists(const std::string& key);

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn);

        muduo::net::InetAddress listenAddr;
        uint64_t casUnique;
        muduo::net::TcpServer server;
    
        std::map<std::string, std::unique_ptr<Session>> sessions;
        std::map<std::string, std::shared_ptr<Item>> items;
};

#endif
