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
    std::cout << "Publication begins...\n";
    for(int i=0; i<100; i++) {        
        auto msg = std::make_unique<message>();
        msg->surprise("Test");
        msg->index(i);        
        pub.publish(std::move(msg));
        std::cout << "Sent " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));        
        if (node->subscriberCount("MessageTopic") == 0) {
            break;
        }
    }    
    return 0;
}
