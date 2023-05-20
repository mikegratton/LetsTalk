#pragma once
#include <cstdint>
#include <cstring>
#include <iosfwd>

#include "FastDdsAlias.hpp"

namespace lt {
/**
 * @brief Unique identifer attached to a message
 *
 * Guid is a unique identifier attached to each message. Guids are
 * 16 byte identifiers of the host, participant, pub/sub, and writer/reader
 * plus an extra 8 byte sequence number that is incremented.
 */
struct Guid {
    unsigned char data[16];
    uint64_t sequence;

    Guid()
    {
        memset(data, 0, 16);
        sequence = 0;
    }

    /// A Guid representing unknown data
    static Guid UNKNOWN() { return Guid(); }

    /// Increment the sequence part of the guid, return a copy of the update
    Guid increment()
    {
        sequence += 1;
        if (sequence == 0) sequence = 1;
        return *this;
    }

    /// Returns the Guid associated with failures
    Guid makeBadVersion() const;

    /// Check if this Guid is the bad version of other
    bool isBadVersionOf(Guid const& i_other) const;
};

/////////////////////////////////////////////////////////////////////////////////
// Inline comparison operators for Guids
inline bool operator<(Guid const& g1, Guid const& g2)
{
    int compare = memcmp(g1.data, g2.data, sizeof(g1.data));
    return (compare < 0 || (compare == 0 && g1.sequence < g2.sequence));
}

inline bool operator==(Guid const& g1, Guid const& g2)
{
    return memcmp(g1.data, g2.data, sizeof(g1.data)) == 0 && g1.sequence == g2.sequence;
}

inline bool operator!=(Guid const& g1, Guid const& g2)
{
    return !(g1 == g2);
}

/// Convert to GUID_t. Drops the sequence data
efr::GUID_t toFastDdsGuid(Guid const& i_guid);

/// Convert from GUID_t. Sets the sequence to zero
Guid toLetsTalkGuid(efr::GUID_t const& i_guid);

/// Convert to SampleIdentity. This is 1-1
efr::SampleIdentity toSampleId(Guid const& i_id);

/// Convert from SampleIdentity. This is 1-1
Guid toLetsTalkGuid(efr::SampleIdentity const& i_sampleId);

std::ostream& operator<<(std::ostream& os, Guid const& i_guid);
}  // namespace lt