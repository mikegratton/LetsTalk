#include "LetsTalk.hpp"
#include <iostream>
#include "HelloWorld.h"

int main(int, char**)
{
    
    
    auto node = lt::Participant::create();
    node->subscribe<HelloWorld>("HelloWorldTopic", [](std::unique_ptr<HelloWorld> data) {
        std::cout << data->message() << " " << data->index() << std::endl;
    }); 

    for ( ; ; ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));            
    }
    return 0;
}
