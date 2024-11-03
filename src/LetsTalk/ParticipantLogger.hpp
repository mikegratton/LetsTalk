#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.hpp>
#include <functional>

#include "FastDdsAlias.hpp"

namespace lt {

class Participant;

namespace detail {

/**
 * ParticipantLogger logs messages about participant discovery in verbose mode
 */
class ParticipantLogger : public efd::DomainParticipantListener {
   public:
    using Callback =
        std::function<void(efd::DomainParticipant* i_participant, efr::ParticipantBuiltinTopicData const& i_info)>;

    ParticipantLogger(std::weak_ptr<Participant> i_participant);

    void on_participant_discovery(efd::DomainParticipant* i_participant, efr::ParticipantDiscoveryStatus i_info,
                                  efr::ParticipantBuiltinTopicData const& info, bool& should_be_ignored) final;

    void on_subscription_matched(efd::DataReader* i_reader, efd::SubscriptionMatchedStatus const& info) final;

    void on_publication_matched(efd::DataWriter* i_writer, efd::PublicationMatchedStatus const& i_info) final;

   protected:
    std::weak_ptr<Participant> m_participant;
    Callback m_callback;
};
}  // namespace detail
}  // namespace lt