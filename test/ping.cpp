#include "LetsTalk.hpp"
#include <iostream>
#include "idl/message.h"

int main(int argc, char** argv)
{
    auto node = lt::Participant::create();
    auto pub = node->advertise<message>("MessageTopic");
    while(node->subscriberCount("MessageTopic") == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    auto msg = std::make_unique<message>();
    msg->surprise("Test");
    msg->index(1);
    pub.publish(std::move(msg));
    return 0;
}
