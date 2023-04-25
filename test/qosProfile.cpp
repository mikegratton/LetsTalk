#include "HelloWorld.h"
#include "LetsTalk.hpp"
#include "doctest.h"

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
