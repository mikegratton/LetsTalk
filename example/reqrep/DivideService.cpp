#include "DivideService.h"

#include <exception>
#include <stdexcept>
#include <thread>

#include "LetsTalk/LetsTalk.hpp"

DivideResult doDivision(DivideProblem const& i_problem)
{
    if (i_problem.denominator() == 0) { throw std::runtime_error("Denominator is zero"); }
    DivideResult result;
    result.quotient(i_problem.numerator() / i_problem.denominator());
    return result;
}

int main(int argc, char** argv)
{
    auto node = lt::Participant::create();
    node->advertise<DivideProblem, DivideResult>("divide_service", doDivision);
    std::this_thread::sleep_for(std::chrono::hours(24));
    return 0;
}
