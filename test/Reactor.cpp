#include <iostream>

#include "HelloWorld.h"
#include "LetsTalk.hpp"
#include "doctest.h"

using namespace lt;

TEST_CASE("Reactor")
{
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    auto SayHi = part1->makeReactorServer<HelloWorld, HelloWorld>("hello");
    auto AskForGreeting = part2->makeReactorClient<HelloWorld, HelloWorld>("hello");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    HelloWorld message;
    message.message("hello?");
    message.index(0);
    auto clientSession = AskForGreeting.request(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::cout << "Starting" << std::endl;

    CHECK(SayHi.havePendingSession() == true);
    auto serverSession = SayHi.getPendingSession();
    CHECK(serverSession.isAlive() == true);
    CHECK(clientSession.isAlive() == true);
    serverSession.progress(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(clientSession.progress() == 10);
    message.message("Hi hi hi!");
    message.index(1);
    serverSession.reply(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(clientSession.progress() == 100);
    HelloWorld rep;
    REQUIRE(clientSession.get(rep));
    CHECK(rep.index() == 1);
}

TEST_CASE("ReactorCancel")
{
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    auto SayHi = part1->makeReactorServer<HelloWorld, HelloWorld>("hello");
    auto AskForGreeting = part2->makeReactorClient<HelloWorld, HelloWorld>("hello");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    HelloWorld message;
    message.message("hello?");
    message.index(0);
    auto clientSession = AskForGreeting.request(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(SayHi.havePendingSession() == true);
    auto serverSession = SayHi.getPendingSession();
    CHECK(serverSession.isAlive() == true);
    CHECK(clientSession.isAlive() == true);

    clientSession.cancel();
    CHECK(clientSession.isAlive() == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(serverSession.isAlive() == false);
}
