#include "item.h"

#include "muduo/base/Timestamp.h"

Item::Item(const std::string& key, const std::string& value, 
        uint16_t flags, uint32_t expireTime, uint64_t cas) 
    : key(key), value(value), flags(flags), expireTime(expireTime), casUnique(cas) {
}

void Item::set(const std::string& value, uint16_t flags, uint32_t exptime, uint64_t cas) {
    this->value = value;
    this->flags = flags;
    this->expireTime = exptime;
    casUnique = cas;
}

std::string Item::get() {
    return value;
}

std::pair<std::string, uint64_t> Item::gets() {
    return std::make_pair(value, casUnique);
}

void Item::append(const std::string& app, uint64_t cas) {
    value += app;
    casUnique = cas;
}

void Item::prepend(const std::string& pre, uint64_t cas) {
    value = pre + value;
    casUnique = cas;
}

bool Item::isExpire() {
    uint32_t current = static_cast<uint32_t>(muduo::Timestamp::now().secondsSinceEpoch());
    return (expireTime != 0 && current >= expireTime);
}

// 相加后溢出(回绕)
uint64_t Item::incr(uint64_t increment, uint64_t cas) {
    uint64_t v = std::stoull(value) + increment;
    value = std::to_string(v);
    casUnique = cas;

    return v;
}

uint64_t Item::decr(uint64_t decrement, uint64_t cas) {
    uint64_t num = std::stoull(value);
    uint64_t v = 0;
    if(decrement < num) {
        v = std::stoull(value) - decrement;
    }
    value = std::to_string(v);
    casUnique = cas;

    return v;
}

void Item::touch(uint32_t expireTime) {
    this->expireTime = expireTime;
}
