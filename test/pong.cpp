#include "LetsTalk.hpp"
#include "idl/message.h"
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    auto node = lt::Participant::create();
    node->subscribe<message>("MessageTopic", [](std::unique_ptr<message> data) {
        std::cout << data->surprise() << " " << data->index() << std::endl;
    });
    std::cout << "Have type " << node->topicType("MessageTopic") << " on topic MessageTopic\n";
    auto sub = node->getRawSubscriber();
    auto listen = sub->lookup_datareader("MessageTopic")->get_listener();
    std::cout << "Listener is " << listen << "\n";
    sub->notify_datareaders();
    while (node->publisherCount("MessageTopic") >= 0 ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));    
    }      
    return 0;
}
