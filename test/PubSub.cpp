#include <chrono>
#include <thread>

#include "LetsTalk.hpp"
#include "doctest.h"
#include "idl/HelloWorld.h"

/*
 * TCP, UDP
 *
 * reliable, bulk, stateful
 */

TEST_CASE("BulkProfile")
{
    auto participant = lt::Participant::create();
    participant->subscribe<HelloWorld>(
        "HelloWorldTopic",
        [](HelloWorld const& data) { std::cout << data.message() << " " << data.index() << std::endl; }, "bulk");

    auto participant2 = lt::Participant::create();
    auto publisher = participant2->advertise<HelloWorld>("HelloWorldTopic", "bulk");
    HelloWorld sample;
    sample.message("hello");
    sample.index(0);
    while (participant2->subscriberCount("HelloWorldTopic") == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    publisher.publish(sample);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_CASE("StatefulProfile")
{
    auto participant2 = lt::Participant::create();
    auto publisher = participant2->advertise<HelloWorld>("HelloWorldTopic", "stateful", 20);
    HelloWorld sample;
    sample.message("hello");
    sample.index(0);
    publisher.publish(sample);
    sample.message("hello");
    sample.index(1);
    publisher.publish(sample);
    sample.message("hello");
    sample.index(2);
    publisher.publish(sample);
    sample.message("hello");
    sample.index(3);
    publisher.publish(sample);
    auto publisher2 = participant2->advertise<HelloWorld>("HelloWorldTopic", "stateful", 20);
    sample.message("goodbye");
    sample.index(0);
    publisher2.publish(std::move(sample));
    sample.message("goodbye");
    sample.index(1);
    publisher2.publish(std::move(sample));
    sample.message("goodbye");
    sample.index(2);
    publisher2.publish(std::move(sample));
    sample.message("goodbye");
    sample.index(3);
    publisher2.publish(std::move(sample));

    auto participant = lt::Participant::create();
    participant->subscribe<HelloWorld>(
        "HelloWorldTopic",
        [](HelloWorld const& data) { std::cout << data.message() << " " << data.index() << std::endl; }, "stateful", 6);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_CASE("UptrOperation")
{
    auto participant = lt::Participant::create();
    auto publisher = participant->advertise<HelloWorld>("HelloWorldTopic");
    auto participant2 = lt::Participant::create();
    participant2->subscribe<HelloWorld>("HelloWorldTopic", [](std::unique_ptr<HelloWorld> ptr) {
        REQUIRE(ptr != nullptr);
        CHECK(ptr->index() == 7);
    });

    while (participant->subscriberCount("HelloWorldTopic") == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    HelloWorld sample;
    sample.index(7);
    publisher.publish(sample);
    std::unique_ptr<HelloWorld> samplePtr(new HelloWorld);
    samplePtr->index(7);
    publisher.publish(std::move(samplePtr));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}