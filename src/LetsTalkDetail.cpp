#include <cstdlib>
#include <iostream>

#include "LetsTalk.hpp"

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

namespace lt {

namespace detail {

/*
 * Turn the mangled name into somethign human readable
 */
#if defined(__GNUC__) || defined(__clang__)
std::string demangle_name(char const* i_mangled)
{
    char* realname = abi::__cxa_demangle(i_mangled, nullptr, nullptr, nullptr);
    std::string rname(realname);
    free(realname);
    return rname;
}
#else
std::string demangle_name(char const* i_mangled)
{
    return std::string(i_mangled);
}
#endif

void logSampleRejected(efd::DataReader* i_reader, const efd::SampleRejectedStatus& status)
{
    if (status.last_reason != 0) {
        LT_LOG << i_reader->get_subscriber()->get_participant() << " rejected sample on \""
               << i_reader->get_topicdescription()->get_name() << "\"; total rejected = " << status.total_count << "\n";
    }
}

void logIncompatibleQos(efd::DataReader* i_reader, const efd::RequestedIncompatibleQosStatus& status)
{
    LT_LOG << i_reader->get_subscriber()->get_participant() << " rejected incompatible QoS match on \""
           << i_reader->get_topicdescription()->get_name() << "\"\n";
}

void logLostSample(efd::DataReader* i_reader, const efd::SampleLostStatus& status)
{
    LT_LOG << i_reader->get_subscriber()->get_participant() << " lost sample on \""
           << i_reader->get_topicdescription()->get_name() << "\"; total lost = " << status.total_count << "\n";
}

std::string requestName(std::string i_name)
{
    return (i_name + "/request");
}

std::string replyName(std::string i_name)
{
    return (i_name + "/reply");
}

/*
 * Set the verbose flag from the environment before main()
 */
bool check_verbose()
{
    char* verb = getenv("LT_VERBOSE");
    if (verb && atoi(verb)) { return true; }
    return false;
}

const bool LT_VERBOSE = check_verbose();

// Check if remote connections allowed
bool getIgnoreNonlocal()
{
    char* verb = getenv("LT_LOCAL_ONLY");
    if (verb && atoi(verb)) { return true; }
    return false;
}

const bool s_IGNORE_NONLOCAL = getIgnoreNonlocal();

}  // namespace detail

}  // namespace lt

// Why is ReturnCode_t so needlessly complicated?
namespace eprosima {
namespace fastrtps {
namespace types {

std::ostream& operator<<(std::ostream& os, ReturnCode_t i_return)
{
    /*
    enum ReturnCodeValue
    {
        RETCODE_OK = 0,
        RETCODE_ERROR = 1,
        RETCODE_UNSUPPORTED = 2,
        RETCODE_BAD_PARAMETER = 3,
        RETCODE_PRECONDITION_NOT_MET = 4,
        RETCODE_OUT_OF_RESOURCES = 5,
        RETCODE_NOT_ENABLED = 6,
        RETCODE_IMMUTABLE_POLICY = 7,
        RETCODE_INCONSISTENT_POLICY = 8,
        RETCODE_ALREADY_DELETED = 9,
        RETCODE_TIMEOUT = 10,
        RETCODE_NO_DATA = 11,
        RETCODE_ILLEGAL_OPERATION = 12,
        RETCODE_NOT_ALLOWED_BY_SECURITY = 13
    };
    */
    switch (i_return()) {
        case 0: return os << "OKAY";
        case 1: return os << "ERROR";
        case 2: return os << "UNSUPPORTED";
        case 3: return os << "BAD PARAMETER";
        case 4: return os << "PRECONDITION NOT MET";
        case 5: return os << "OUT OF RESOURCES";
        case 6: return os << "NOT ENABLED";
        case 7: return os << "IMMUTABLE POLICY";
        case 8: return os << "INCONSISTENT POLICY";
        case 9: return os << "ALREADY DELETED";
        case 10: return os << "TIMEOUT";
        case 11: return os << "NO DATA";
        case 12: return os << "ILLEGAL OPERATION";
        case 13: return os << "NOT ALLOWED BY SECURITY";
        default: return os << "UNKNOWN (" << i_return() << ")";
    }
}
}  // namespace types
}  // namespace fastrtps
}  // namespace eprosima
