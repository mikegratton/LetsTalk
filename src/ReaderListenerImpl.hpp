#pragma once
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "LetsTalk.hpp"
#include "ParticipantImpl.hpp"
namespace lt {
namespace detail {

//////////////////////////////////////////////////////////////////////////////
// ReaderListener class template implementation
// This is in detail because instances should not be visible outside the library
template <class T, class C>
ReaderListener<T, C>::ReaderListener(C i_callback) : m_callback(i_callback)
{
}

template <class T, class C>
void ReaderListener<T, C>::on_data_available(efd::DataReader* i_reader)
{
    typename functor_tagger<C, T>::type tag;
    if (!handle_sample(i_reader, tag)) {
        LT_LOG << i_reader->get_subscriber()->get_participant() << " " << i_reader->get_topicdescription()->get_name()
               << " callback has an incomplete sample.\n";
    }
}

template <class T, class C>
bool ReaderListener<T, C>::handle_sample(efd::DataReader* i_reader, wants_guid_tag)
{
    T data;
    efd::SampleInfo info;
    while (ReturnCode_t::RETCODE_OK == i_reader->take_next_sample(&data, &info)) {
        if (info.valid_data) {
            m_callback(data, toLetsTalkGuid(info.sample_identity), toLetsTalkGuid(info.related_sample_identity));
            return true;
        }
    }
    return false;
}

template <class T, class C>
bool ReaderListener<T, C>::handle_sample(efd::DataReader* i_reader, plain_tag)
{
    T data;
    efd::SampleInfo info;
    while (ReturnCode_t::RETCODE_OK == i_reader->take_next_sample(&data, &info)) {
        if (info.valid_data) {
            m_callback(data);
            return true;
        }
    }
    return false;
}

template <class T, class C>
bool ReaderListener<T, C>::handle_sample(efd::DataReader* i_reader, uptr_with_guid_tag)
{
    std::unique_ptr<T> data(new T);
    efd::SampleInfo info;
    while (ReturnCode_t::RETCODE_OK == i_reader->take_next_sample(&data, &info)) {
        if (info.valid_data) {
            m_callback(std::move(data), toLetsTalkGuid(info.sample_identity),
                       toLetsTalkGuid(info.related_sample_identity));
            return true;
        }
    }
    return false;
}

template <class T, class C>
bool ReaderListener<T, C>::handle_sample(efd::DataReader* i_reader, uptr_tag)
{
    std::unique_ptr<T> data(new T);
    efd::SampleInfo info;
    while (ReturnCode_t::RETCODE_OK == i_reader->take_next_sample(&data, &info)) {
        if (info.valid_data) {
            m_callback(std::move(data));
            return true;
        }
    }
    return false;
}

template <class T, class C>
void ReaderListener<T, C>::on_sample_rejected(efd::DataReader* reader, const efd::SampleRejectedStatus& status)
{
    logSampleRejected(reader, status);
}

template <class T, class C>
void ReaderListener<T, C>::on_requested_incompatible_qos(efd::DataReader* reader,
                                                         const efd::RequestedIncompatibleQosStatus& status)
{
    logIncompatibleQos(reader, status);
}

template <class T, class C>
void ReaderListener<T, C>::on_sample_lost(efd::DataReader* reader, const efd::SampleLostStatus& status)
{
    logLostSample(reader, status);
}
}  // namespace detail
}  // namespace lt