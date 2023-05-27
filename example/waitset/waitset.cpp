#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "letstalk/QueueWaitset.hpp"

auto intQueue = std::make_shared<lt::ThreadSafeQueue<int>>();
auto stringQueue = std::make_shared<lt::ThreadSafeQueue<std::string>>();
std::atomic<bool> keepAlive = true;

int main(int argc, char** argv)
{
    auto waitset = lt::makeQueueWaitset(intQueue, stringQueue);

    std::thread produce([]() {
        while (keepAlive) {
            double r = drand48();
            if (r < 0.2) {
                intQueue->emplace(5);
            } else if (r < 0.4) {
                stringQueue->emplace("hello");
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });

    for (;;) {
        int triggerIndex = waitset.wait();
        switch (triggerIndex) {
            case 0: {
                auto content = intQueue->popAll();
                if (content.size() > 0) {
                    std::cout << "Int queue had data: ";
                    for (auto const& p : content) { std::cout << *p << " "; }
                    std::cout << "\n";
                }
            }
            // Note fallthrough
            case 1: {
                auto content = stringQueue->popAll();
                if (content.size() > 0) {
                    std::cout << "String queue had data: ";
                    for (auto const& p : content) { std::cout << *p << " "; }
                    std::cout << "\n";
                }
            }
        }
    }

    return 0;
}