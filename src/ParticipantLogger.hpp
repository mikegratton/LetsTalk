#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <functional>

#include "FastDdsAlias.hpp"

namespace lt {

class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;

namespace detail {

/**
 * ParticipantLogger logs messages about participant discovery in verbose mode
 */
class ParticipantLogger : public efd::DomainParticipantListener {
   public:
    using Callback = std::function<void(efd::DomainParticipant* i_participant, efr::ParticipantDiscoveryInfo&& i_info)>;

    ParticipantLogger(ParticipantPtr i_participant);

    void on_participant_discovery(efd::DomainParticipant* i_participant, efr::ParticipantDiscoveryInfo&& i_info) final;

    void on_subscription_matched(efd::DataReader* i_reader, efd::SubscriptionMatchedStatus const& info) final;

    void on_publication_matched(efd::DataWriter* i_writer, efd::PublicationMatchedStatus const& i_info) final;

   protected:
    ParticipantPtr m_participant;
    Callback m_callback;
};
}  // namespace detail
}  // namespace lt