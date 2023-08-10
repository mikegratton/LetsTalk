#include <chrono>
#include <iostream>
#include <thread>

#include "LetsTalk/LetsTalk.hpp"
#include "OpenGarage.h"

int main(int argc, char** argv)
{
    lt::ParticipantPtr node = lt::Participant::create();
    auto garageService = node->makeReactorServer<ChangeRequest, ChangeResult, ChangeStatus>("garage_door");
    std::cout << "Waiting for requests\n";
    while (true) {
        auto serverSession = garageService.getPendingSession();
        if (serverSession.isAlive() == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::cout << "Request for status: " << serverSession.request().desired_state() << "\n";
        ChangeStatus status;
        status.current_state(GarageDoorState::kopening);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        serverSession.progress(20, status);
        std::cout << "Progress " << 20 << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        serverSession.progress(40, status);
        std::cout << "Progress " << 40 << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        serverSession.progress(60, status);
        std::cout << "Progress " << 60 << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        serverSession.progress(80, status);
        std::cout << "Progress " << 80 << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ChangeResult result;
        result.final_state(GarageDoorState::kopen);
        serverSession.reply(result);
        std::cout << "Finished with result " << result.final_state() << "\n";
    }
    return 0;
}