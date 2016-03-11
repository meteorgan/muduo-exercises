#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <thread>
#include <chrono>

#include "memcachedClient.h"

struct MemcachedClientStart{
    MemcachedClientStart() :client("127.0.0.1", 11211) { client.connect(); };
    ~MemcachedClientStart() {};

    MemcachedClient client;
};

BOOST_FIXTURE_TEST_SUITE(MemcachedClientSuite, MemcachedClientStart);

BOOST_AUTO_TEST_CASE(setAndGet) {
    client.set("key1", "value1");
    BOOST_REQUIRE_EQUAL(client.get("key1"), "value1");

    client.set("key2", "value2");
    client.set("key3", "value3", 1);
    BOOST_REQUIRE_EQUAL(client.get("key2"), "value2");
    BOOST_REQUIRE_EQUAL(client.get("key3"), "value3");

    std::this_thread::sleep_for(std::chrono::seconds(2));
    BOOST_REQUIRE_THROW(client.get("key3"), std::runtime_error);

    BOOST_REQUIRE_THROW(client.get("setAndGetNotExistKey"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(deleteKey) {
    client.add("key1", "value1");
    BOOST_REQUIRE_EQUAL(client.deleteKey("key1"), true);
    BOOST_REQUIRE_EQUAL(client.deleteKey("key1"), false);
}

BOOST_AUTO_TEST_CASE(add) {
    client.set("key1", "value1");
    BOOST_REQUIRE_EQUAL(client.add("key1", "value1"), false);

    client.deleteKey("key1");
    BOOST_REQUIRE_EQUAL(client.add("key1", "add"), true);
    BOOST_REQUIRE_EQUAL(client.get("key1"), "add");
}

BOOST_AUTO_TEST_CASE(replace) {
    client.deleteKey("key1");
    BOOST_REQUIRE_EQUAL(client.replace("key1", "replace"), false);

    client.set("key1", "value1");
    BOOST_REQUIRE_EQUAL(client.replace("key1", "replace"), true);
    BOOST_REQUIRE_EQUAL(client.get("key1"), "replace");
}

BOOST_AUTO_TEST_CASE(append) {
    client.deleteKey("key1");
    BOOST_REQUIRE_EQUAL(client.append("key1", "append"), false);

    client.add("key1", "append");
    BOOST_REQUIRE_EQUAL(client.append("key1", "app"), true);
    BOOST_REQUIRE_EQUAL(client.get("key1"), "appendapp");
}

BOOST_AUTO_TEST_CASE(prepend) {
    client.deleteKey("key1");
    BOOST_REQUIRE_EQUAL(client.prepend("key1", "pre"), false);

    client.add("key1", "prepend");
    BOOST_REQUIRE_EQUAL(client.prepend("key1", "pre"), true);
    BOOST_REQUIRE_EQUAL(client.get("key1"), "preprepend");
}

BOOST_AUTO_TEST_CASE(getLong) {
    client.deleteKey("key1");
    BOOST_REQUIRE_THROW(client.getLong("key1"), std::runtime_error);

    client.set("key1", "10");
    BOOST_REQUIRE_EQUAL(client.getLong("key1"), 10);

    client.set("key1", "xxx");
    BOOST_REQUIRE_THROW(client.getLong("key1"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(gets) {
    client.set("key1", "gets");
    std::pair<std::string, int64_t> p = client.gets("key1");
    BOOST_REQUIRE_EQUAL(p.first, "gets");
}

BOOST_AUTO_TEST_CASE(getsLong) {
    client.set("key1", "10");
    std::pair<uint64_t, int64_t> p = client.getsLong("key1");
    BOOST_REQUIRE_EQUAL(p.first, 10);
}

BOOST_AUTO_TEST_CASE(incr) {
    client.deleteKey("key1");
    BOOST_REQUIRE_THROW(client.incr("key1", 10), std::runtime_error);

    client.set("key1", "10");
    BOOST_REQUIRE_EQUAL(client.incr("key1", 1), 11);

    client.set("key1", "incr");
    BOOST_REQUIRE_THROW(client.incr("key1", 1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(decr) {
    client.deleteKey("key1");
    BOOST_REQUIRE_THROW(client.decr("key1", 1), std::runtime_error);

    client.add("key1", "10");
    BOOST_REQUIRE_EQUAL(client.decr("key1", 2), 8);

    client.append("key1", "xxx");
    BOOST_REQUIRE_THROW(client.decr("key1", 1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(touch) {
    client.deleteKey("key1");
    BOOST_REQUIRE_EQUAL(client.touch("key1", 10), false);

    client.set("key1", "touch", 100);
    BOOST_REQUIRE_EQUAL(client.touch("key1", 1), true); 
    std::this_thread::sleep_for(std::chrono::seconds(2));
    BOOST_REQUIRE_THROW(client.get("key1"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(cas) {
    client.deleteKey("key1");
    BOOST_REQUIRE_THROW(client.cas("key1", "cas", 1), std::runtime_error);

    client.add("key1", "cas");
    std::pair<std::string, int64_t> p = client.gets("key1");
    BOOST_REQUIRE_EQUAL(client.cas("key1", "cas1", p.second), true);
    BOOST_REQUIRE_EQUAL(client.get("key1"), "cas1");

    client.set("key1", "set");
    BOOST_REQUIRE_EQUAL(client.cas("key1", "cas2", p.second), false);
    BOOST_REQUIRE_EQUAL(client.get("key1"), "set");
}

BOOST_AUTO_TEST_CASE(getMulti) {
    client.deleteKey("key1");
    client.deleteKey("key2");
    client.deleteKey("key3");
    client.deleteKey("key4");

    std::vector<std::string> keys{"key1", "key2", "key3", "key4"};
    BOOST_REQUIRE_EQUAL(client.getMulti(keys).size(), 0);

    client.add("key2", "value2");
    std::map<std::string, std::string> values1 = client.getMulti(keys);
    BOOST_REQUIRE_EQUAL(values1.size(), 1);
    BOOST_REQUIRE_EQUAL(values1["key2"], "value2");

    client.add("key3", "value3");
    auto values2 = client.getMulti(keys);
    BOOST_REQUIRE_EQUAL(values2.size(), 2);
    BOOST_REQUIRE_EQUAL(values2["key3"], "value3");

    client.add("key1", "value1");
    client.add("key4", "value4");
    auto values3 = client.getMulti(keys);
    BOOST_REQUIRE_EQUAL(values3.size(), 4);
    BOOST_REQUIRE_EQUAL(values3["key1"], "value1");
    BOOST_REQUIRE_EQUAL(values3["key2"], "value2");
}

BOOST_AUTO_TEST_CASE(getsMulti) {
    std::vector<std::string> keys{"key1", "key2", "key3"};
    for(auto& key : keys) {
        client.deleteKey(key);
    }

    BOOST_REQUIRE_EQUAL(client.getsMulti(keys).size(), 0);

    client.add("key2", "value2");
    auto values1 = client.getsMulti(keys);
    BOOST_REQUIRE_EQUAL(values1.size(), 1);
    BOOST_REQUIRE_EQUAL(values1["key2"].first, "value2");

    client.add("key1", "value1");
    client.add("key3", "value3");
    auto values2 = client.getsMulti(keys);
    BOOST_REQUIRE_EQUAL(values2.size(), 3);
    BOOST_REQUIRE_EQUAL(values2["key1"].first, "value1");
    BOOST_REQUIRE_EQUAL(values2["key3"].first, "value3");
}


BOOST_AUTO_TEST_SUITE_END();
