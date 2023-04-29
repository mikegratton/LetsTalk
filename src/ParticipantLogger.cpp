#include "ParticipantLogger.hpp"

#include "LetsTalk.hpp"
namespace lt {
namespace detail {

ParticipantLogger::ParticipantLogger(ParticipantPtr i_participant)
    : m_participant(i_participant), m_callback([](efd::DomainParticipant*, efr::ParticipantDiscoveryInfo&&) {})
{
}

void ParticipantLogger::on_participant_discovery(efd::DomainParticipant* i_participant,
                                                 efr::ParticipantDiscoveryInfo&& i_info)
{
    if (i_info.status == efr::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT) {
        LT_LOG << i_participant << " discovered participant \"" << i_info.info.m_participantName << "\"\n";
        bool isLocal = i_info.info.m_guid.is_on_same_host_as(i_participant->guid());
        if (s_IGNORE_NONLOCAL && !isLocal) {
            LT_LOG << i_participant << " ignored remote participant \"" << i_info.info.m_participantName << "\"\n";
            i_participant->ignore_participant(i_info.info.m_key);
        }
        m_callback(i_participant, std::move(i_info));
    } else if (i_info.status == efr::ParticipantDiscoveryInfo::CHANGED_QOS_PARTICIPANT) {
        LT_LOG << i_participant << " saw participant \"" << i_info.info.m_participantName << "\" change qos\n";
    } else {
        LT_LOG << i_participant << " lost participant \"" << i_info.info.m_participantName << "\"\n";
    }
}

void ParticipantLogger::on_publication_matched(efd::DataWriter* i_writer, efd::PublicationMatchedStatus const& info)
{
    auto const& topic = i_writer->get_topic()->get_name();
    auto* participant = i_writer->get_publisher()->get_participant();
    if (m_participant) { m_participant->updateSubscriberCount(topic, info.current_count_change); }
    if (info.current_count_change > 0) {
        LT_LOG << participant << " topic \"" << topic << "\" matched " << info.current_count << " subscriber(s)\n";
    } else {
        LT_LOG << participant << " topic \"" << topic << "\" lost " << -info.current_count_change << " subscriber(s)\n";
    }
}

void ParticipantLogger::on_subscription_matched(efd::DataReader* i_reader, efd::SubscriptionMatchedStatus const& i_info)
{
    auto const& topic = i_reader->get_topicdescription()->get_name();
    if (m_participant) { m_participant->updatePublisherCount(topic, i_info.current_count_change); }
    if (i_info.current_count_change > 0) {
        LT_LOG << i_reader->get_subscriber()->get_participant() << " topic \"" << topic << "\" matched "
               << i_info.current_count << " publisher(s)\n";
    } else {
        LT_LOG << i_reader->get_subscriber()->get_participant() << " topic \"" << topic << "\" lost "
               << -i_info.current_count_change << " publisher(s)\n";
    }
}
}  // namespace detail
}  // namespace lt