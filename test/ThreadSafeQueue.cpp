#include "LetsTalk/ThreadSafeQueue.hpp"

#include <chrono>

#include "doctest.h"

TEST_CASE("QueueBasics")
{
    lt::ThreadSafeQueue<std::string> queue;
    REQUIRE(queue.empty() == true);
    REQUIRE(queue.size() == 0);
    auto item = queue.pop();
    CHECK(nullptr == item);
    auto items = queue.popAll();
    CHECK(items.empty() == true);
    queue.emplace("goodbye");
    queue.clear();
    REQUIRE(queue.empty() == true);

    auto p = std::unique_ptr<std::string>(new std::string());
    *p = "hello";
    queue.push(std::move(p));
    REQUIRE(queue.empty() == false);
    REQUIRE(queue.size() == 1);
    item = queue.pop();
    REQUIRE(nullptr != item);
    CHECK(*item == "hello");

    // popAll
    p = std::unique_ptr<std::string>(new std::string());
    *p = "world";
    queue.push(std::move(p));
    items = queue.popAll();
    REQUIRE(items.size() == 1);
    CHECK(*items.front() == "world");

    // pushAll
    lt::ThreadSafeQueue<std::string> queue2;
    lt::ThreadSafeQueue<std::string>::Queue bulk;
    bulk.emplace_back(new std::string("one"));
    bulk.emplace_back(new std::string("two"));
    bulk.emplace_back(new std::string("three"));
    queue.clear();
    queue.pushAll(std::move(bulk));
    CHECK(bulk.size() == 0);
    CHECK(queue.size() == 3);
    CHECK(*queue.pop() == "one");
    CHECK(*queue.pop() == "two");
    CHECK(*queue.pop() == "three");
    CHECK(queue.size() == 0);

    // swap
    bulk.emplace_back(new std::string("one"));
    bulk.emplace_back(new std::string("two"));
    bulk.emplace_back(new std::string("three"));
    queue.clear();
    queue.pushAll(std::move(bulk));
    queue.swap(queue2);
    CHECK(queue.size() == 0);
    CHECK(queue2.size() == 3);
    CHECK(*queue2.pop() == "one");
    CHECK(*queue2.pop() == "two");
    CHECK(*queue2.pop() == "three");
    CHECK(queue2.size() == 0);
}

TEST_CASE("BoundedQueue")
{
    lt::ThreadSafeQueue<std::string> queue(3);
    CHECK(queue.capacity() == 3);
    queue.emplace("one");
    queue.emplace("two");
    queue.emplace("three");
    queue.emplace("four");
    CHECK(queue.size() == 3);
    CHECK(*queue.pop() == "two");

    lt::ThreadSafeQueue<std::string>::Queue bulk;
    bulk.emplace_back(new std::string("alpha"));
    bulk.emplace_back(new std::string("beta"));
    bulk.emplace_back(new std::string("gamma"));
    queue.pushAll(std::move(bulk));
    CHECK(queue.size() == 3);
    CHECK(*queue.pop() == "alpha");
}

TEST_CASE("QueueUntil")
{
    lt::ThreadSafeQueue<std::string> queue;
    queue.push("hello");
    queue.push("world");
    auto data = queue.popAll(std::chrono::steady_clock::now() + std::chrono::nanoseconds(-100));
    CHECK(data.size() == 2);
    auto startTime = std::chrono::steady_clock::now();
    data = queue.popAll(startTime + std::chrono::milliseconds(100));
    std::chrono::nanoseconds elapsed = std::chrono::steady_clock::now() - startTime;
    CHECK(elapsed < std::chrono::milliseconds(150));
    CHECK(data.size() == 0);
}