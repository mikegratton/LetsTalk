#include "LetsTalk.hpp"
#include <iostream>
#include "HelloWorld.h"

int main(int argc, char** argv)
{
    auto node = lt::Participant::create();
    auto pub = node->advertise<HelloWorld>("HelloWorldTopic");
    while(node->subscriberCount("HelloWorldTopic") == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Publication begins...\n";
    for(int i=0; i<100; i++) {
        auto msg = std::make_unique<HelloWorld>();
        msg->message("Test");
        msg->index(i);
        bool okay = pub.publish(std::move(msg));
        std::cout << "Sent " << i << "  " << (okay? "okay" : "FAILED") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (node->subscriberCount("HelloWorldTopic") == 0 || !okay) {
            break;
        }
    }
    return 0;
}
