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

BOOST_AUTO_TEST_SUITE_END();
