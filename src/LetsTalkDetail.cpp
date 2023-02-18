#include "LetsTalk.hpp"
#include <cstdlib>
#include <iostream>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif



namespace lt {

namespace detail {


#if defined(__GNUC__) || defined(__clang__)
std::string demangle_name(char const* i_mangled) {
    char* realname = abi::__cxa_demangle(i_mangled, nullptr, nullptr, nullptr);
    std::string rname(realname);
    free(realname);
    return rname;
}
#else
std::string demangle_name(char const* i_mangled) {
    return std::string(i_mangled);
}
#endif

efr::GUID_t toNative(Guid const& i_guid)
{
    efr::GUID_t fastGuid;    
    memcpy(fastGuid.guidPrefix.value, i_guid.data, efr::GuidPrefix_t::size);
    memcpy(fastGuid.entityId.value, &i_guid.data[12], efr::EntityId_t::size);
    return fastGuid;    
}

Guid toLib(efr::GUID_t const& i_guid)
{
    Guid lib;
    memcpy(lib.data, i_guid.guidPrefix.value, efr::GuidPrefix_t::size);
    memcpy(&lib.data[efr::GuidPrefix_t::size], i_guid.entityId.value, i_guid.entityId.size);
    return lib;
}

efr::SampleIdentity toSampleId(Guid const& i_id)
{
    efr::SampleIdentity id;
    id.writer_guid(toNative(i_id));
    id.sequence_number(efr::SequenceNumber_t(i_id.sequence));    
    return id;
}

Guid fromSampleId(efr::SampleIdentity const& i_sampleId)
{
    Guid guid = toLib(i_sampleId.writer_guid());
    guid.sequence = i_sampleId.sequence_number().to64long();    
    return guid;
}

bool getIgnoreNonlocal() {
    char* verb = getenv("LT_LOCAL_ONLY");
    if (verb && atoi(verb)) {
        return true;
    }
    return false;
}

const bool s_IGNORE_NONLOCAL = getIgnoreNonlocal();


ParticipantLogger::ParticipantLogger(ParticipantPtr i_participant)
    : m_participant(i_participant)
    , m_callback([](efd::DomainParticipant*, efr::ParticipantDiscoveryInfo &&) {})
{ }

void ParticipantLogger::on_participant_discovery(efd::DomainParticipant* i_participant,
        efr::ParticipantDiscoveryInfo&& i_info) {
    if (i_info.status == efr::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT) {
        LT_LOG << i_participant << " discovered participant \""
               << i_info.info.m_participantName << "\"\n";
        bool isLocal = i_info.info.m_guid.is_on_same_host_as(i_participant->guid());
        if (s_IGNORE_NONLOCAL && !isLocal) {
            LT_LOG << i_participant << " ignored remote participant \""
                   << i_info.info.m_participantName << "\"\n";
            i_participant->ignore_participant(i_info.info.m_key);
        }
        m_callback(i_participant, std::move(i_info));
    } else if (i_info.status == efr::ParticipantDiscoveryInfo::CHANGED_QOS_PARTICIPANT) {
        LT_LOG << i_participant << " saw participant \"" << i_info.info.m_participantName
               << "\" change qos\n";
    } else {
        LT_LOG << i_participant << " lost participant \""
               << i_info.info.m_participantName << "\"\n";
    }
}

void ParticipantLogger::on_publication_matched(efd::DataWriter* i_writer,
        efd::PublicationMatchedStatus const& info) {

    auto const& topic = i_writer->get_topic()->get_name();
    auto* participant = i_writer->get_publisher()->get_participant();
    if (m_participant) {
        m_participant->updateSubscriberCount(topic, info.current_count_change);
    }
    if (info.current_count_change > 0) {
        LT_LOG << participant
               << " topic \"" << topic << "\" matched "
               << info.current_count << " subscriber(s)\n";
    } else {
        LT_LOG << participant
               << " topic \"" << topic << "\" lost "
               << -info.current_count_change << " subscriber(s)\n";
    }
}


void ParticipantLogger::on_subscription_matched(efd::DataReader* i_reader,
        efd::SubscriptionMatchedStatus const& i_info) {
    auto const& topic = i_reader->get_topicdescription()->get_name();
    if (m_participant) {
        m_participant->updatePublisherCount(topic, i_info.current_count_change);
    }
    if (i_info.current_count_change > 0) {
        LT_LOG << i_reader->get_subscriber()->get_participant()
               << " topic \"" << topic << "\" matched "
               << i_info.current_count << " publisher(s)\n";
    } else {
        LT_LOG << i_reader->get_subscriber()->get_participant()
               << " topic \"" << topic << "\" lost "
               << -i_info.current_count_change << " publisher(s)\n";
    }
}


void logSampleRejected(efd::DataReader* i_reader, const efd::SampleRejectedStatus& status) {
    if (status.last_reason != 0) {
        LT_LOG << i_reader->get_subscriber()->get_participant()  << " rejected sample on \""
               << i_reader->get_topicdescription()->get_name()
               << "\"; total rejected = " << status.total_count << "\n";
    }
}

void logIncompatibleQos(efd::DataReader* i_reader,
                        const efd::RequestedIncompatibleQosStatus& status) {

    LT_LOG << i_reader->get_subscriber()->get_participant()
           << " rejected incompatible QoS match on \""
           << i_reader->get_topicdescription()->get_name()  << "\"\n";
}

void logLostSample(efd::DataReader* i_reader, const efd::SampleLostStatus& status) {
    LT_LOG << i_reader->get_subscriber()->get_participant()  << " lost sample on \""
           << i_reader->get_topicdescription()->get_name()  << "\"; total lost = "
           << status.total_count << "\n";
}


std::string requestName(std::string i_name) {
    return (i_name + "/request");
}


std::string replyName(std::string i_name) {
    return (i_name + "/reply");
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

efr::Time_t getBadTime()
{
    return efr::Time_t(1,1);
}

} // detail

} // lt


// Why is ReturnCode_t so needlessly complicated?
namespace eprosima {
namespace fastrtps {
namespace types {

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
}
}
}
