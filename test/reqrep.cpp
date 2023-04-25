#include "HelloWorld.h"
#include "LetsTalk.hpp"
#include "doctest.h"

TEST_CASE("FailedRequest")
{
  lt::ParticipantPtr p1 = lt::Participant::create();
  p1->advertise<HelloWorld, HelloWorld>("greet", [](HelloWorld const& req) -> HelloWorld const& {
    (void)req;
    throw std::runtime_error("Error!");
  });

  lt::ParticipantPtr p2 = lt::Participant::create();
  auto requester = p2->request<HelloWorld, HelloWorld>("greet");
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
}

TEST_CASE("BasicRequest")
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
  auto requester = p2->request<HelloWorld, HelloWorld>("greet");

  lt::ParticipantPtr p3 = lt::Participant::create();
  auto requester2 = p3->request<HelloWorld, HelloWorld>("greet");

  while (!requester.isConnected() && !requester2.isConnected()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  HelloWorld req;
  req.index(1);
  req.message("Goodbye");
  auto rep2 = requester2.request(req);

  req.index(0);
  req.message("Hello");
  auto reply = requester.request(req);
  try {
    auto rep = reply.get();
    std::cout << "Reply: " << rep.message() << " " << rep.index() << "\n";
    CHECK(rep.index() == 0);
  } catch (...) {
    std::cout << "Failed request\n";
    CHECK(true == false);
  }

  auto rep = rep2.get();
  std::cout << "Reply: " << rep.message() << " " << rep.index() << "\n";
  CHECK(rep.index() == 1);
}

TEST_CASE("PingPongRequest")
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
  auto requester = p2->request<HelloWorld, HelloWorld>("greet");
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
    } catch (std::runtime_error const& e) {
      std::cout << "Service sent failure response\n";
    } catch (std::exception const& e) {
      std::cout << "Caught wrong type of exception: " << e.what() << "\n";
      CHECK(1 == 0);
    }
  }
}
