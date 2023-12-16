#include <iostream>

#include "HelloWorld.h"
#include "HelloWorldJsonSupport.h"
#include "LetsTalk/LetsTalk.hpp"

int main(int, char**)
{
    auto node = lt::Participant::create();
    node->subscribe<HelloWorld>("HelloWorldTopic", [](HelloWorld const& data) {
        std::string j = HelloWorldToJson(data);
        std::cout << j << std::endl;
        auto data2 = HelloWorldFromJson(j);
    });
    for (;;) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    return 0;
}
