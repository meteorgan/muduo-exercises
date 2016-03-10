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


BOOST_AUTO_TEST_SUITE_END();
