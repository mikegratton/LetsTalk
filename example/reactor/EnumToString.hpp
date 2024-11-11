#pragma once
#include <iostream>

#include "idl/OpenGarage.hpp"

inline std::ostream& operator<<(std::ostream& os, GarageDoorState const& i_state)
{
    switch (i_state) {
        case GarageDoorState::kopen: return os << "OPEN";
        case GarageDoorState::kclosed: return os << "CLOSED";
        case GarageDoorState::kstuck: return os << "STUCK";
        case GarageDoorState::kopening: return os << "OPENING";
        case GarageDoorState::kclosing: return os << "CLOSING";
        default: return os << "UNKNOWN";
    }
}
