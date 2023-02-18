#include "Reactor.hpp"

namespace lt
{
std::string reactorCommandName(std::string const& i_name)
{
    return i_name + "/command";
}

std::string reactorProgressName(std::string const& i_name)
{
    return i_name + "/progress";
}

std::string reactorReplyName(std::string const& i_name)
{
    return i_name + "/reply";
}

std::string reactorRequestName(std::string const& i_name)
{
    return i_name + "/request";
}
}
