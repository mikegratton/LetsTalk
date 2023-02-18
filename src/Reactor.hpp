#pragma once

#include <string>
#include "Reactor.h"

namespace lt
{

std::string reactorCommandName(std::string const& i_name);
std::string reactorProgressName(std::string const& i_name);
std::string reactorReplyName(std::string const& i_name);
std::string reactorRequestName(std::string const& i_name);


enum ReactorProgressMark {
    PROG_UNKNOWN = -200,
    PROG_FAILED = -100,
    PROG_SENT = 0,
    PROG_START = 1,
    PROG_SUCCESS = 100
};

}
