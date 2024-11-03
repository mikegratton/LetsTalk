#include <iostream>

#include "HelloWorld.hpp"
#include "LetsTalk/LetsTalk.hpp"

int main(int, char**)
{
    auto node = lt::Participant::create();
    auto messageQueue = node->subscribe<HelloWorld>("HelloWorldTopic");

    for (;;) {
        // If a message is available in the next 10 ms, messagePtr will be non-null
        auto messagePtr = messageQueue->pop(std::chrono::milliseconds(10));
        if (messagePtr) { std::cout << messagePtr->message() << " : " << messagePtr->index() << "\n"; }
    }

    return 0;
}
