#ifndef MEMCACHED_ITEM_H
#define MEMCACHED_ITEM_H

#include <iostream>

class Item {
    public:
        Item(const std::string& key, const std::string& value,
                uint16_t flags, uint32_t expireTime, uint64_t cas);

        void set(const std::string& newValue, uint64_t cas);

        std::string get();

        std::pair<std::string, uint64_t> gets();

        void append(const std::string& app, uint64_t cas);

        void prepend(const std::string& pre, uint64_t cas);

        uint64_t incr(uint64_t increment);

        uint64_t decr(uint64_t decrement);

        bool isExpire();

    private:
        const uint32_t maxExpireTime = 2592000;
        std::string key;
        std::string value;
        uint16_t flags;
        uint32_t expireTime;
        uint64_t casUnique;
        uint32_t expireTimestamp;
};

#endif
