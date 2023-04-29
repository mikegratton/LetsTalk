#pragma once
#include <fastdds/rtps/common/all_common.h>

#include <iosfwd>

/**
 * Namespace aliases and the like for interacting with fastdds
 */

namespace lt {

//////////////////////////////////////////////////
// Namespace aliases
namespace efd = eprosima::fastdds::dds;
namespace efr = eprosima::fastrtps::rtps;
//////////////////////////////////////////////////
}  // namespace lt

// FastDDS return codes to string translation
namespace eprosima {
namespace fastrtps {
namespace types {
struct ReturnCode_t;  // Oh no
std::ostream& operator<<(std::ostream& os, ReturnCode_t i_return);
}  // namespace types
}  // namespace fastrtps
}  // namespace eprosima
