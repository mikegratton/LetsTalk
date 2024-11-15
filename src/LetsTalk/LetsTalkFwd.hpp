#pragma once
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <memory>

#include "ActiveObject.hpp"
#include "FastDdsAlias.hpp"
#include "Guid.hpp"
#include "ParticipantLogger.hpp"
#include "ReaderListener.hpp"

namespace lt {

//////////////////////////////////////////////////
// Forward declarations
class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;

class Publisher;

template <class Req, class Rep>
class Requester;

namespace detail {
class RequesterImplBase {
   public:
    virtual ~RequesterImplBase() = default;
};
using RequesterImplPtr = std::shared_ptr<RequesterImplBase>;

// Check if remote connections allowed
extern const bool s_IGNORE_NONLOCAL;

// Logging is controlled by this variable, set from the environment on startup
extern const bool LT_VERBOSE;

// The log macro swallows the message is LT_VERBOSE is false
#define LT_LOG                       \
    if (!::lt::detail::LT_VERBOSE) { \
    } else                           \
        std::cout

////////////////////////////////////////////////////////////////////////////////////////

void logSampleRejected(efd::DataReader* i_reader, const efd::SampleRejectedStatus& i_status);
void logIncompatibleQos(efd::DataReader* i_reader, const efd::RequestedIncompatibleQosStatus& i_status);
void logLostSample(efd::DataReader* i_reader, const efd::SampleLostStatus& i_status);

}  // namespace detail
}  // namespace lt
