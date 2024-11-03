#pragma once
#include <fastdds/dds/core/detail/DDSReturnCode.hpp>
#include <fastdds/rtps/common/Guid.hpp>
#include <fastdds/rtps/common/SampleIdentity.hpp>
#include <iosfwd>

/**
 * Namespace aliases and the like for interacting with fastdds
 */

namespace lt {

//////////////////////////////////////////////////
// Namespace aliases
namespace efd = eprosima::fastdds::dds;
namespace efr = eprosima::fastdds::rtps;

char const* returnCodeToString(efd::ReturnCode_t i_return);

//////////////////////////////////////////////////
}  // namespace lt
