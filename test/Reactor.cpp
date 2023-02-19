#include "doctest.h"
#include "ReactorClient.hpp"
#include "ReactorServer.hpp"
#include "HelloWorld.h"
#include <iostream>

using namespace lt;

TEST_CASE("Reactor")
{
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    ReactorServer<HelloWorld, HelloWorld> SayHi(part1, "hello");
    ReactorClient<HelloWorld, HelloWorld> AskForGreeting(part2, "hello");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto message = std::unique_ptr<HelloWorld>(new HelloWorld);
    message->message("hello?");
    message->index(0);
    auto clientSession = AskForGreeting.request(std::move(message));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    CHECK(SayHi.havePendingSession() == true);
    auto serverSession = SayHi.getPendingSession();
    CHECK(serverSession.isAlive() == true);
    CHECK(clientSession.isAlive() == true);
    serverSession.progress(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(clientSession.progress() == 10);
    message = std::unique_ptr<HelloWorld>(new HelloWorld);
    message->message("Hi hi hi!");
    message->index(1);
    serverSession.reply(std::move(message));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(clientSession.progress() == 100);
    auto rep = clientSession.get();
    REQUIRE(rep != nullptr);
    CHECK(rep->index() == 1);
}

TEST_CASE("ReactorCancel")
{
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    ReactorServer<HelloWorld, HelloWorld> SayHi(part1, "hello");
    ReactorClient<HelloWorld, HelloWorld> AskForGreeting(part2, "hello");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto message = std::unique_ptr<HelloWorld>(new HelloWorld);
    message->message("hello?");
    message->index(0);
    auto clientSession = AskForGreeting.request(std::move(message));
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
