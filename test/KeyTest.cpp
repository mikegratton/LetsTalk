#include "LetsTalk/LetsTalk.hpp"
#include "LetsTalk/PubSubType.hpp"
#include "doctest.h"
#include "idl/Big.hpp"

// This just needs to compile to show that the compute key code
// can be found through the template helper class
TEST_CASE("Key.PubSub")
{
    modname::Big sample;
    lt::detail::PubSubType<modname::Big> pubsub;
    sample.keymember(1);
}