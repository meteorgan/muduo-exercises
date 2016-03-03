#ifndef MEMCACHED_CLIENT_H
#define MEMCACHED_CLIENT_H

#include "muduo/net/TcpClient.h"

#include <iostream>
#include <vector>

class MemcachedClient {
    public:
        MemcachedClient(std::string serverIP, uint16_t port);

        void set(std::string key, std::string value, uint32_t expire = 0);

        bool add(std::string key, std::string value, uint32_t expire = 0);

        bool replace(std::string key, std::string value, uint32_t expire = 0);

        bool append(std::string key, std::string value);

        bool prepend(std::string key, std::string value);

        bool cas(std::string key, std::string value, unsigned long casUnique, uint32_t expire = 0);

        std::string get(std::string key);

        // return: <value, cas unique>
        std::pair<std::string, unsigned long> gets(std::string key);

        std::vector<std::string> getMulti(std::string key, ...);

        unsigned long getLong(std::string key);

        // return: <value, case unique>
        std::pair<unsigned long, unsigned long> getsLong(std::string key);

        unsigned long incr(std::string key, unsigned long value);

        unsigned long decr(std::string key, unsigned long value);

        bool deleteKey(std::string key);

        bool touch(std::string key, uint32_t expire);

    private:

}; 

#endif
