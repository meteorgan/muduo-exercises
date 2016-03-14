#ifndef MEMCACHED_H
#define MEMCACHED_H

#include "muduo/net/TcpConnection.h"
#include "muduo/net/TcpServer.h"

#include "item.h"

#include <set>

class Memcached {
    public:
        Memcached(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr);

        void start();

        void set();
       
        void add();

        void replace();

        void append();

        void prepend();

        void cas();

        void get();

        void gets();

        void deleteKey();

        void incr();

        void decr();

        void touch();

        void stats();

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn);

        muduo::net::InetAddress listenAddr;
        uint64_t casUnique;
        muduo::net::TcpServer server;
    
        std::set<Item> items;
};

#endif
