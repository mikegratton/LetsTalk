#include "doctest.h"
#include "LetsTalk.hpp"
#include "HelloWorld.h"

TEST_CASE("BasicRequest")
{
    lt::ParticipantPtr p1 = lt::Participant::create();
    p1->advertise<HelloWorld, HelloWorld>("greet",
        [](std::unique_ptr<HelloWorld> req) -> std::unique_ptr<HelloWorld> {
            std::cout << "Request: " << req->message() << " " << req->index() << "\n";
            auto rep = std::unique_ptr<HelloWorld>(new HelloWorld);
            rep->index(req->index());
            rep->message("World");
            return rep;
        });
    
    lt::ParticipantPtr p2 = lt::Participant::create();
    auto requester = p2->request<HelloWorld, HelloWorld>("greet");
    
    lt::ParticipantPtr p3 = lt::Participant::create();
    auto requester2 = p3->request<HelloWorld, HelloWorld>("greet");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto req = std::unique_ptr<HelloWorld>(new HelloWorld);
    req->index(1);
    req->message("Goodbye");
    auto rep2 = requester2.request(std::move(req));
    
    req = std::unique_ptr<HelloWorld>(new HelloWorld);
    req->index(0);
    req->message("Hello");
    auto reply = requester.request(std::move(req));
    try {
        auto rep = reply.get();
        std::cout << "Reply: " << rep->message() << " " << rep->index() << "\n";
        CHECK(rep->index() == 0);
    } catch (...) {
        std::cout << "Failed request\n";
        CHECK(true == false);
    }
    
    auto rep = rep2.get();
    std::cout << "Reply: " << rep->message() << " " << rep->index() << "\n";
    CHECK(rep->index() == 1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("FailedRequest")
{
    lt::ParticipantPtr p1 = lt::Participant::create();
    p1->advertise<HelloWorld, HelloWorld>("greet",
        [](std::unique_ptr<HelloWorld> req) -> std::unique_ptr<HelloWorld> {
            (void) req;
            throw std::runtime_error("Error!");            
        });
    
    lt::ParticipantPtr p2 = lt::Participant::create();
    auto requester = p2->request<HelloWorld, HelloWorld>("greet");
    auto req = std::unique_ptr<HelloWorld>(new HelloWorld);
    req->index(0);
    req->message("Hello");
    auto reply = requester.request(std::move(req));
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
}
