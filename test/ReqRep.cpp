#include <chrono>

#include "LetsTalk/LetsTalk.hpp"
#include "doctest.h"
#include "idl/HelloWorld.hpp"

TEST_CASE("Request.Failed")
{
    lt::ParticipantPtr p1 = lt::Participant::create();
    p1->advertise<HelloWorld, HelloWorld>("greet", [](HelloWorld const& req) -> HelloWorld const& {
        std::cout << "!!!!! Failing request\n";
        throw std::runtime_error("Error!");
    });

    lt::ParticipantPtr p2 = lt::Participant::create();
    auto requester = p2->makeRequester<HelloWorld, HelloWorld>("greet");

    // Wait to connect
    while (!requester.isConnected()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }

    HelloWorld req;
    req.index(0);
    req.message("Hello");
    auto reply = requester.request(req);
    try {
        auto rep = reply.get();
        CHECK(1 == 0);
    } catch (std::runtime_error const& e) {
        std::cout << "Failed request (on purpose)\n";
        CHECK(true == true);
    } catch (std::exception const& e) {
        std::cout << "Caught wrong type of exception: " << e.what() << "\n";
        CHECK(1 == 0);
    }
    p1->unadvertise("greet");
}

TEST_CASE("Request.Basic")
{
    lt::ParticipantPtr p1 = lt::Participant::create();
    p1->advertise<HelloWorld, HelloWorld>("greet", [](HelloWorld const& req) -> HelloWorld {
        std::cout << "Request: " << req.message() << " " << req.index() << "\n";
        HelloWorld rep;
        rep.index(req.index());
        rep.message("World");
        return rep;
    });

    lt::ParticipantPtr p2 = lt::Participant::create();
    auto requester = p2->makeRequester<HelloWorld, HelloWorld>("greet");

    lt::ParticipantPtr p3 = lt::Participant::create();
    auto requester2 = p3->makeRequester<HelloWorld, HelloWorld>("greet");

    while (!requester.isConnected() && !requester2.isConnected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    HelloWorld req;
    req.index(1);
    req.message("Goodbye");
    auto rep2 = requester2.request(req);

    HelloWorld req2;
    req2.index(0);
    req2.message("Hello");
    auto reply = requester.request(req2);
    try {
        REQUIRE(reply.valid());
        reply.wait();
        auto rep = reply.get();
        std::cout << "Reply: " << rep.message() << " " << rep.index() << "\n";
        CHECK(rep.index() == 0);
    } catch (std::exception const& e) {
        std::cout << "Failed request: " << e.what() << "\n";
        CHECK(true == false);
    }
    try {
        auto rep = rep2.get();
        std::cout << "Reply: " << rep.message() << " " << rep.index() << "\n";
        CHECK(rep.index() == 1);
    } catch (std::exception const& e) {
        std::cout << "Failed request: " << e.what() << "\n";
        CHECK(true == false);
    }
    p1->unadvertise("greet");
    requester.~Requester();
    requester2.~Requester();

    std::cout << "Use count p1: " << p1.use_count() << ", p2: " << p2.use_count() << "\n";
    p1.reset();
    p2.reset();
}

TEST_CASE("Request.PingPong")
{
    lt::ParticipantPtr p1 = lt::Participant::create();
    p1->advertise<HelloWorld, HelloWorld>("greet", [](HelloWorld const& req) -> HelloWorld {
        std::cout << "Request " << req.index() << ": " << req.message() << "\n";
        if (req.index() % 2) { throw std::runtime_error("Error!"); }
        HelloWorld reply;
        reply.index(req.index());
        reply.message("World");
        return reply;
    });

    lt::ParticipantPtr p2 = lt::Participant::create();
    auto requester = p2->makeRequester<HelloWorld, HelloWorld>("greet");

    // Wait to connect
    while (!requester.isConnected()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }

    for (int i = 0; i < 5; i++) {
        HelloWorld req;
        req.index(i);
        req.message("Hello");
        auto reply = requester.request(req);
        try {
            auto rep = reply.get();
            std::cout << "Reply " << rep.index() << ": " << rep.message() << "\n";
            CHECK(true);
        } catch (std::runtime_error const& e) {
            std::cout << "Service sent failure response\n";
        } catch (std::exception const& e) {
            std::cout << "Caught wrong type of exception: " << e.what() << "\n";
            CHECK(1 == 0);
        }
    }
    p1->unadvertise("greet");
}

TEST_CASE("Request.Pull")
{
    lt::ParticipantPtr p1 = lt::Participant::create();
    auto replier = p1->advertise<HelloWorld, HelloWorld>("greet");
    lt::ParticipantPtr p2 = lt::Participant::create();
    auto requester = p2->makeRequester<HelloWorld, HelloWorld>("greet");
    // Wait to connect
    // while (!requester.isConnected()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }

    // Spam requests
    std::future<HelloWorld> futures[4];
    for (int i = 0; i < 4; i++) {
        HelloWorld req;
        req.index(i);
        req.message("Hello");
        futures[i] = p2->request<HelloWorld, HelloWorld>("greet", req);
    }

    // Process requests
    for (int i = 0; i < 4; i++) {
        auto session = replier.getPendingSession(std::chrono::milliseconds(100));
        CHECK(session.isAlive());
        CHECK(session.request().index() == i);
        HelloWorld rep = session.request();
        rep.message("World");
        session.reply(rep);
    }
    auto session = replier.getPendingSession(std::chrono::milliseconds(1));
    CHECK(session.isAlive() == false);

    for (int i = 0; i < 4; i++) {
        auto reply = futures[i].get();
        CHECK(reply.index() == i);
    }
    HelloWorld req;
    req.index(10);
    req.message("Won't work");
    auto failure = requester.request(req);
    session = replier.getPendingSession(std::chrono::milliseconds(100));
    CHECK(session.request().index() == 10);
    session.fail();
    try {
        auto nope = failure.get();
        CHECK(nope.index() != 0);
    } catch (...) {
    }
}