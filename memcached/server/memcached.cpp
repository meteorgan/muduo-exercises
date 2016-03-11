#include "memcached.h"

Memcached::Memcached(muduo::net::EventLoop* loop, 
        const muduo::net::InetAddress& listenAddr) 
    : listenAddr(listenAddr), casUnique(0) {
}
