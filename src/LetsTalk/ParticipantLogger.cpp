#include "ParticipantLogger.hpp"

#include <memory>

#include "LetsTalk.hpp"
#include "fastdds/rtps/builtin/data/ParticipantBuiltinTopicData.hpp"
namespace lt {
namespace detail {

ParticipantLogger::ParticipantLogger(std::weak_ptr<Participant> i_participant)
    : m_participant(i_participant), m_callback([](efd::DomainParticipant*, efr::ParticipantBuiltinTopicData const&) {})
{
}

void ParticipantLogger::on_participant_discovery(efd::DomainParticipant* i_participant,
                                                 efr::ParticipantDiscoveryStatus status,
                                                 efr::ParticipantBuiltinTopicData const& i_info,
                                                 bool& should_be_ignored)
{
    if (status == efr::ParticipantDiscoveryStatus::DISCOVERED_PARTICIPANT) {
        LT_LOG << i_participant << " discovered participant \"" << i_info.participant_name << "\"\n";
        bool isLocal = i_info.guid.is_on_same_host_as(i_participant->guid());
        if (s_IGNORE_NONLOCAL && !isLocal) {
            LT_LOG << i_participant << " ignored remote participant \"" << i_info.participant_name << "\"\n";
            i_participant->ignore_participant(i_participant->get_instance_handle());
        }
        m_callback(i_participant, i_info);
    } else if (status == efr::ParticipantDiscoveryStatus::CHANGED_QOS_PARTICIPANT) {
        LT_LOG << i_participant << " saw participant \"" << i_info.participant_name << "\" change qos\n";
    } else {
        LT_LOG << i_participant << " lost participant \"" << i_info.participant_name << "\"\n";
    }
}

void ParticipantLogger::on_publication_matched(efd::DataWriter* i_writer, efd::PublicationMatchedStatus const& info)
{
    auto const& topic = i_writer->get_topic()->get_name();
    auto* participant = i_writer->get_publisher()->get_participant();
    std::shared_ptr<Participant> lockedLtParticipant = m_participant.lock();
    if (lockedLtParticipant) { lockedLtParticipant->updateSubscriberCount(topic, info.current_count_change); }
    if (info.current_count_change > 0) {
        LT_LOG << participant << " topic \"" << topic << "\" matched " << info.current_count << " subscriber(s)\n";
    } else {
        LT_LOG << participant << " topic \"" << topic << "\" lost " << -info.current_count_change << " subscriber(s)\n";
    }
}

void ParticipantLogger::on_subscription_matched(efd::DataReader* i_reader, efd::SubscriptionMatchedStatus const& i_info)
{
    auto const& topic = i_reader->get_topicdescription()->get_name();
    std::shared_ptr<Participant> lockedLtParticipant = m_participant.lock();
    if (lockedLtParticipant) { lockedLtParticipant->updatePublisherCount(topic, i_info.current_count_change); }
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