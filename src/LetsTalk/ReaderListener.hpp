#pragma once
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

#include "FastDdsAlias.hpp"
#include "meta.hpp"

namespace lt {
namespace detail {

/**
 * ReaderListener adapts a simple callback for use with fastdds. These are created internally
 * by Participant.
 */
template <class T, class C>
class ReaderListener : public efd::DataReaderListener {
   public:
    using type = T;

    ReaderListener(C i_callback);

    void on_data_available(efd::DataReader* i_reader) final;

    void on_sample_rejected(efd::DataReader* i_reader, const efd::SampleRejectedStatus& i_status) final;

    void on_requested_incompatible_qos(efd::DataReader* i_reader,
                                       const efd::RequestedIncompatibleQosStatus& i_status) final;

    void on_sample_lost(efd::DataReader* i_reader, const efd::SampleLostStatus& i_status) final;

   protected:
    /// Tag dispatched handle_sample calls C with the deduced arguments
    bool handle_sample(efd::DataReader* i_reader, wants_guid_tag);
    bool handle_sample(efd::DataReader* i_reader, plain_tag);
    bool handle_sample(efd::DataReader* i_reader, uptr_with_guid_tag);
    bool handle_sample(efd::DataReader* i_reader, uptr_tag);

    C m_callback;
};

/**
 * Helper function to create listener instances from callbacks
 */
template <class T, class C>
efd::DataReaderListener* makeListener(C i_callback)
{
    return new ReaderListener<T, C>(i_callback);
}
}  // namespace detail
}  // namespace lt