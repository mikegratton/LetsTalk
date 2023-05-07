#include <chrono>
#include <iostream>
#include <thread>

#include "LetsTalk.hpp"
#include "doctest.h"
#include "idl/HelloWorld.h"
#include "idl/OpenGarage.h"

using namespace lt;

TEST_CASE("Reactor.Basic")
{
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    auto SayHi = part1->makeReactorServer<HelloWorld, HelloWorld>("hello");
    auto AskForGreeting = part2->makeReactorClient<HelloWorld, HelloWorld>("hello");

    while (!AskForGreeting.discoveredServer()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

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

TEST_CASE("Reactor.Cancel")
{
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    auto SayHi = part1->makeReactorServer<HelloWorld, HelloWorld>("hello");
    auto AskForGreeting = part2->makeReactorClient<HelloWorld, HelloWorld>("hello");

    while (!AskForGreeting.discoveredServer()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

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

TEST_CASE("Reactor.CustomProgress")
{
    using namespace lt;
    ParticipantPtr part1 = Participant::create();
    ParticipantPtr part2 = Participant::create();
    auto garageService = part1->makeReactorServer<ChangeRequest, ChangeResult, ChangeStatus>("garage_door");
    auto garageRemote = part2->makeReactorClient<ChangeRequest, ChangeResult, ChangeStatus>("garage_door");

    while (!garageRemote.discoveredServer()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

    ChangeRequest request;
    request.desired_state(GarageDoorState::kopen);
    auto clientSession = garageRemote.request(request);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto serverSession = garageService.getPendingSession(std::chrono::seconds(3));
    std::cout << "This session is " << serverSession.id() << "\n";
    REQUIRE(serverSession.isAlive() == true);
    CHECK(serverSession.request().desired_state() == GarageDoorState::kopen);
    ChangeStatus status;
    status.current_state(GarageDoorState::kopening);

    serverSession.progress(10, status);
    status.current_state(GarageDoorState::kstuck);
    serverSession.progress(11, status);
    status.current_state(GarageDoorState::kopening);
    serverSession.progress(90, status);
    REQUIRE(clientSession.isAlive());
    CHECK(clientSession.progress() >= PROG_SENT);
    ChangeStatus deliveredStatus;
    CHECK(clientSession.progressData(deliveredStatus));
    CHECK(deliveredStatus.current_state() == GarageDoorState::kopening);
    CHECK(clientSession.progressData(deliveredStatus));
    CHECK(deliveredStatus.current_state() == GarageDoorState::kstuck);
    CHECK(clientSession.progressData(deliveredStatus));
    CHECK(deliveredStatus.current_state() == GarageDoorState::kopening);
    CHECK(clientSession.progress() == 90);
    ChangeResult result;
    result.final_state(GarageDoorState::kopen);
    serverSession.reply(result);
    ChangeResult clientResult;
    CHECK(clientSession.get(clientResult));
    CHECK(clientResult.final_state() == GarageDoorState::kopen);
    CHECK(clientSession.progress() == PROG_SUCCESS);
}