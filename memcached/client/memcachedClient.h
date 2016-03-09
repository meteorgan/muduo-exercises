#ifndef MEMCACHED_CLIENT_H
#define MEMCACHED_CLIENT_H

#include <unistd.h>

#include <iostream>
#include <vector>
#include <map>

class MemcachedClient {
    public:
        MemcachedClient(std::string serverIP, uint16_t port)
            : serverIP(serverIP), port(port), fd(0) {}

        MemcachedClient(MemcachedClient& mc) = delete;

        MemcachedClient& operator=(MemcachedClient& mc) = delete;

        ~MemcachedClient() {
            if(fd != 0) {
                close(fd);
            }
        }

        void connect();

        bool set(std::string key, std::string value, uint32_t expire = 0);

        bool add(std::string key, std::string value, uint32_t expire = 0);

        bool replace(std::string key, std::string value, uint32_t expire = 0);

        bool append(std::string key, std::string value);

        bool prepend(std::string key, std::string value);

        bool cas(std::string key, std::string value, unsigned long casUnique, uint32_t expire = 0);

        std::string get(std::string key);

        unsigned long getLong(std::string key);

        std::map<std::string, std::string> getMulti(std::vector<std::string>& keys);

        // return: <value, cas unique>
        std::pair<std::string, unsigned long> gets(std::string key);

        // return: <value, case unique>
        std::pair<unsigned long, unsigned long> getsLong(std::string key);

        std::map<std::string, std::pair<std::string, unsigned long>> getsMulti(std::vector<std::string>& keys);

        uint64_t incr(std::string key, uint64_t value);

        uint64_t decr(std::string key, uint64_t value);

        bool deleteKey(std::string key);

        bool touch(std::string key, uint32_t expire);

    private:
        bool sendStorageCommand(std::string command, std::string key, std::string value, uint32_t expire = 0);
        void sendRequest(const std::string data);
        bool sendRequest(const std::string& value, std::string expectReponse, std::string failResponse);

        std::string readLine(size_t maxlen);
        std::string readBytes(size_t size);

        std::string serverIP;
        uint16_t port;
        int fd;
}; 

#endif
