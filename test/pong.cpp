#include <iostream>

#include "HelloWorld.h"
#include "LetsTalk.hpp"

int main(int, char**)
{
  auto node = lt::Participant::create();
  node->subscribe<HelloWorld>("HelloWorldTopic", [](HelloWorld const& data) {
    std::cout << data.message() << " " << data.index() << std::endl;
  });

  for (;;) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
  return 0;
}
