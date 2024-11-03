#include <cstdlib>
#include <iostream>

#include "LetsTalk.hpp"
#include "fastdds/dds/core/detail/DDSReturnCode.hpp"
#include "fastdds/dds/core/detail/DDSSecurityReturnCode.hpp"

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

char const* returnCodeToString(efd::ReturnCode_t i_return)
{
    switch (i_return) {
        case efd::RETCODE_OK: return "OKAY";
        case efd::RETCODE_ERROR: return "ERROR";
        case efd::RETCODE_UNSUPPORTED: return "UNSUPPORTED";
        case efd::RETCODE_BAD_PARAMETER: return "BAD PARAMETER";
        case efd::RETCODE_PRECONDITION_NOT_MET: return "PRECONDITION NOT MET";
        case efd::RETCODE_OUT_OF_RESOURCES: return "OUT OF RESOURCES";
        case efd::RETCODE_NOT_ENABLED: return "NOT ENABLED";
        case efd::RETCODE_IMMUTABLE_POLICY: return "IMMUTABLE POLICY";
        case efd::RETCODE_INCONSISTENT_POLICY: return "INCONSISTENT POLICY";
        case efd::RETCODE_ALREADY_DELETED: return "ALREADY DELETED";
        case efd::RETCODE_TIMEOUT: return "TIMEOUT";
        case efd::RETCODE_NO_DATA: return "NO DATA";
        case efd::RETCODE_ILLEGAL_OPERATION: return "ILLEGAL OPERATION";
        case efd::RETCODE_NOT_ALLOWED_BY_SECURITY: return "NOT ALLOWED BY SECURITY";
        default: return "UNKNOWN";
    }
}

}  // namespace lt
