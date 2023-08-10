#include <cstdio>
#include <thread>

#include "DivideService.h"
#include "LetsTalk/LetsTalk.hpp"

int main(int argc, char** argv)
{
    lt::ParticipantPtr node = lt::Participant::create();
    lt::Requester<DivideProblem, DivideResult> requester =
        node->makeRequester<DivideProblem, DivideResult>("divide_service");
    while (!requester.isConnected()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

    if (argc != 3) {
        printf("Usage %s <numerator> <denominator>\nCompute numerator/denominator via Let's Talk service call.\n",
               argv[0]);
        return 1;
    }
    DivideProblem problem;
    problem.numerator(atoi(argv[1]));
    problem.denominator(atoi(argv[2]));
    printf("Submitting problem %d/%d ...\n", problem.numerator(), problem.denominator());
    std::future<DivideResult> futureResult = requester.request(problem);

    try {
        DivideResult result = futureResult.get();
        printf("Service computed: %d/%d = %d\n", problem.numerator(), problem.denominator(), result.quotient());
    } catch (std::exception const& e) {
        printf("Service reported an error.\n");
    }
    return 0;
}