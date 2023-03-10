#pragma once

#include <fastdds/rtps/common/SerializedPayload.h>
#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastrtps/utils/md5.h>
    
namespace lt
{

template<class T>
class PubSubType : public eprosima::fastdds::dds::TopicDataType {
public:

    typedef T type;

    PubSubType();

    virtual ~PubSubType() override {
        if (m_keyBuffer != nullptr) {
            free(m_keyBuffer);
        }
    }

    virtual bool serialize(void* data,
                           eprosima::fastrtps::rtps::SerializedPayload_t* payload) override;

    virtual bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t* payload,
                             void* data) override;

    virtual std::function<uint32_t()> getSerializedSizeProvider(void* data) override;

    virtual bool getKey(void* data, eprosima::fastrtps::rtps::InstanceHandle_t* ihandle,
                        bool force_md5 = false) override;

    virtual void* createData() override { return reinterpret_cast<void*>(new T()); }

    virtual void deleteData(void* data) override { delete reinterpret_cast<T*>(data); }

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED
    inline bool is_bounded() const override { return false; }
#endif  // TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_PLAIN
    inline bool is_plain() const override { return false; }
#endif  // TOPIC_DATA_TYPE_API_HAS_IS_PLAIN

#ifdef TOPIC_DATA_TYPE_API_HAS_CONSTRUCT_SAMPLE
    inline bool construct_sample(void* memory) const override { (void)memory; return false; }
#endif  // TOPIC_DATA_TYPE_API_HAS_CONSTRUCT_SAMPLE

    MD5 m_md5;
    unsigned char* m_keyBuffer;
};
}
////////////////////////////////////////////

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include "LetsTalkFwd.hpp"
namespace lt
{
template<class T>
PubSubType<T>::PubSubType() {    
    setName(detail::get_demangled_name<T>().c_str());    
    auto type_size = T::getMaxCdrSerializedSize();
    type_size += eprosima::fastcdr::Cdr::alignment(type_size, 4); /* possible submessage alignment */
    m_typeSize = static_cast<uint32_t>(type_size) + 4; /*encapsulation*/
    m_isGetKeyDefined = T::isKeyDefined();
    size_t keyLength = T::getKeyMaxCdrSerializedSize() > 16 ? T::getKeyMaxCdrSerializedSize() : 16;
    m_keyBuffer = reinterpret_cast<unsigned char*>(malloc(keyLength));
    memset(m_keyBuffer, 0, keyLength);
}

template<class T>
bool PubSubType<T>::serialize(void* data, ::eprosima::fastrtps::rtps::SerializedPayload_t* payload) {
    T* p_type = static_cast<T*>(data);

    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->max_size);
    eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
    payload->encapsulation = ser.endianness() == ::eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
    ser.serialize_encapsulation();

    try {
        // Serialize the object.
        p_type->serialize(ser);
    } catch (eprosima::fastcdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    // Get the serialized length
    payload->length = static_cast<uint32_t>(ser.getSerializedDataLength());
    return true;
}


template<class T>
bool PubSubType<T>::deserialize(::eprosima::fastrtps::rtps::SerializedPayload_t* payload, void* data) {
    try {
        T* p_type = static_cast<T*>(data);
        eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->length);
        eprosima::fastcdr::Cdr deser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
        deser.read_encapsulation();
        payload->encapsulation = deser.endianness() == eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
        p_type->deserialize(deser);
    } catch (eprosima::fastcdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    return true;
}


template<class T>
inline std::function<uint32_t()> PubSubType<T>::getSerializedSizeProvider(void* data) {
    return [data]() -> uint32_t {
        return static_cast<uint32_t>(type::getCdrSerializedSize(*static_cast<T*>(data))) +
        4u /*encapsulation*/;
    };
}

template<class T>
inline bool PubSubType<T>::getKey(void* data, ::eprosima::fastrtps::rtps::InstanceHandle_t* handle, 
                               bool force_md5) {
    if (!m_isGetKeyDefined) {
        return false;
    }

    T* p_type = static_cast<T*>(data);

    // Object that manages the raw buffer.
    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(m_keyBuffer),
            T::getKeyMaxCdrSerializedSize());

    // Object that serializes the data.
    eprosima::fastcdr::Cdr ser(fastbuffer, ::eprosima::fastcdr::Cdr::BIG_ENDIANNESS);
    p_type->serializeKey(ser);
    if (force_md5 || T::getKeyMaxCdrSerializedSize() > 16) {
        m_md5.init();
        m_md5.update(m_keyBuffer, static_cast<unsigned int>(ser.getSerializedDataLength()));
        m_md5.finalize();
        for (uint8_t i = 0; i < 16; ++i) {
            handle->value[i] = m_md5.digest[i];
        }
    } else {
        for (uint8_t i = 0; i < 16; ++i) {
            handle->value[i] = m_keyBuffer[i];
        }
    }
    return true;
}
}
