#pragma once
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "LetsTalk.hpp"
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
  std::size_t expected = i_reader->get_unread_count();
  for (std::size_t i = 0; i < expected; i++) {
    T data;
    efd::SampleInfo info;
    auto code = i_reader->take_next_sample(&data, &info);
    if (code == ReturnCode_t::RETCODE_OK) {
      if (info.valid_data) { handle_sample(data, info, tag); }
    } else {
      LT_LOG << i_reader->get_subscriber()->get_participant() << " " << i_reader->get_topicdescription()->get_name()
             << " callback has an incomplete sample: " << code << "\n";
      break;
    }
  }
}

template <class T, class C>
void ReaderListener<T, C>::handle_sample(T const& i_sample, efd::SampleInfo const& i_info, wants_guid_tag)
{
  m_callback(i_sample, toLetsTalkGuid(i_info.sample_identity), toLetsTalkGuid(i_info.related_sample_identity));
}

template <class T, class C>
void ReaderListener<T, C>::handle_sample(T const& i_sample, efd::SampleInfo const&, plain_tag)
{
  m_callback(i_sample);
}

template <class T, class C>
void ReaderListener<T, C>::handle_sample(T const& i_sample, efd::SampleInfo const& i_info, uptr_with_guid_tag)
{
  m_callback(std::unique_ptr<T>(new T(i_sample)), toLetsTalkGuid(i_info.sample_identity),
             toLetsTalkGuid(i_info.related_sample_identity));
}

template <class T, class C>
void ReaderListener<T, C>::handle_sample(T const& i_sample, efd::SampleInfo const&, uptr_tag)
{
  m_callback(std::unique_ptr<T>(new T(i_sample)));
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