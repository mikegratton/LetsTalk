#include "LetsTalk.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>


namespace lt {

namespace detail {

std::string name(ParticipantPtr const& p) {
    if (p) {
        std::stringstream ss;
        ss << p.get();
        return ss.str();
    }
    return "UNKNOWN";
}


SubscriberCounter::SubscriberCounter(std::string const& i_topic, ParticipantPtr i_participant)
    : m_topic(i_topic)
    , m_participant(i_participant)
{ }

void SubscriberCounter::on_publication_matched(efd::DataWriter*, efd::PublicationMatchedStatus const& info) {
    if (m_participant) {
        m_participant->updateSubscriberCount(m_topic, info.current_count_change);
    }
    if (info.current_count_change > 0) {
        LT_LOG << m_participant << " topic \"" << m_topic << "\" matched "
               << info.current_count << " subscriber(s)\n";
    } else {
        LT_LOG << m_participant << " topic \"" << m_topic << "\" lost "
               << -info.current_count_change << " subscriber(s)\n";
    }
}

ParticipantLogger::ParticipantLogger() { }

/*
ParticipantLogger::ParticipantLogger()
    : m_callback([](efd::DomainParticipant*
    , efr::ParticipantDiscoveryInfo &&) {})
{ }

void ParticipantLogger::on_participant_discovery(efd::DomainParticipant* i_participant,
        efr::ParticipantDiscoveryInfo&& i_info) {
    if (i_info.status == efr::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT) {
        LT_LOG << i_participant << " discovered participant \""
               << i_info.info.m_participantName << "\"\n";
        // m_callback(i_participant, std::move(i_info));
    } else if (i_info.status == efr::ParticipantDiscoveryInfo::CHANGED_QOS_PARTICIPANT) {
        LT_LOG << i_participant << " saw participant \"" << i_info.info.m_participantName
               << "\" change qos\n";
    } else {
        LT_LOG << i_participant << " lost participant \""
               << i_info.info.m_participantName << "\"\n";
    }
}
*/

void logSubscriptionMatched(ParticipantPtr const& i_participant, std::string const& i_topic,
                            efd::SubscriptionMatchedStatus const& i_info) {
    if (i_info.current_count_change > 0) {
        LT_LOG << name(i_participant) << " topic \"" << i_topic << "\" matched "
               << i_info.current_count << " publisher(s)\n";
    } else {
        LT_LOG << name(i_participant) << " topic \"" << i_topic << "\" lost "
               << -i_info.current_count_change << " publisher(s)\n";
    }
}

void logSampleRejected(ParticipantPtr const& i_participant, std::string const& i_topic,
                       const efd::SampleRejectedStatus& status) {
    if (status.last_reason != 0) {
        LT_LOG << i_participant << " rejected sample on \"" << i_topic << "\"; total rejected = "
               << status.total_count << "\n";
    }
}

void logIncompatibleQos(ParticipantPtr const& i_participant, std::string const& i_topic,
                        const efd::RequestedIncompatibleQosStatus& status) {

    LT_LOG << i_participant << " rejected incompatible QoS match on \"" << i_topic << "\"\n";
}

void logLostSample(ParticipantPtr const& i_participant, std::string const& i_topic,
                   const efd::SampleLostStatus& status) {
    LT_LOG << i_participant << " lost sample on \"" << i_topic << "\"; total lost = "
           << status.total_count << "\n";
}

/*
 * Set the verbose flag from the environment before main()
 */
bool check_verbose() {
    char* verb = getenv("LT_VERBOSE");
    if (verb && atoi(verb)) {
        return true;
    }
    return false;
}

const bool LT_VERBOSE = check_verbose();


}

}


// Why is ReturnCode_t so needlessly complicated?
namespace eprosima { namespace fastrtps { namespace types {

std::ostream& operator<<(std::ostream& os, ReturnCode_t i_return) {
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
} } }
