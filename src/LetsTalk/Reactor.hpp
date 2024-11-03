#pragma once

#include <string>

#include "LetsTalkFwd.hpp"
#include "idl/Reactor.hpp"
/**
 * A reactor is a communication pattern that is like an extended request/reply session.
 *
 * The client sends an initial request,  starting a session. The server then sends back
 * multiple progress messages, delivering both a status counter and optional progress data.
 * Finally, the server sends a reply message, ending the session. During its execution, the
 * client may send a cancel command, ending the session early.
 *
 * Some examples of reactors:
 *
 * A garage door opener service. The client requests the door to be opened, and the server sends back
 * progress related to how open the door is now. The reply type may be blank.  Cancellation could cause
 * the server to try to restore the door to its previous state (open or closed).
 *
 * A route planner service. The client requests a safe route from a start to a goal. The server runs an
 * anytime algorithm with a fixed time budget. Progress messages are sent at a regular interval, but
 * may jump to fully complete if the algorithm solves the problem before the time elapses. The reply is
 * the safe route. The client may cancel the calculation if the upcoming task of the robot has changed.
 */

namespace lt {

/*
 * Canonical names for reactor topics computed from the reactor service name
 */
std::string reactorCommandName(std::string const& i_name);
std::string reactorProgressName(std::string const& i_name);
std::string reactorReplyName(std::string const& i_name);
std::string reactorRequestName(std::string const& i_name);

enum ReactorProgressMark : int {
    PROG_UNKNOWN = -200,
    PROG_FAILED = -100,
    PROG_SENT = 0,
    PROG_START = 1,
    PROG_SUCCESS = 100
};

namespace detail {
template <class Req, class Rep, class ProgressData>
class ReactorClientBackend;
template <class Req, class Rep, class ProgressData>
class ReactorServerBackend;
}  // namespace detail

}  // namespace lt

#include "ReactorClient.hpp"
#include "ReactorServer.hpp"
