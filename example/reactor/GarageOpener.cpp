#include <chrono>
#include <iostream>
#include <thread>

#include "EnumToString.hpp"
#include "LetsTalk/LetsTalk.hpp"
#include "idl/OpenGarage.hpp"

int main(int argc, char** argv)
{
    lt::ParticipantPtr node = lt::Participant::create();
    auto garageOpener = node->makeReactorClient<ChangeRequest, ChangeResult, ChangeStatus>("garage_door");

    // Wait for discovery
    while (!garageOpener.discoveredServer()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

    ChangeRequest request;
    request.desired_state(GarageDoorState::kopen);
    auto session = garageOpener.request(request);
    ChangeResult result;
    std::cout << "Commanding state " << request.desired_state() << "\n";
    while (session.get(result, std::chrono::milliseconds(100)) == false) {
        ChangeStatus status;
        if (session.progressData(status, std::chrono::milliseconds(100))) {
            std::cout << "Progress: " << session.progress() << ", state=" << status.current_state() << "\n";
        }
    }
    std::cout << "Result: " << result.final_state() << "\n";
}
