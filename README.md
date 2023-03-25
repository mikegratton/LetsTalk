Let's Talk: A C++ Interprocess Communication System Based on FastDDS
=================================================

# Introduction

Let's Talk is an API wrapped around the FastDDS library, along with
a distribution of that library, the IDL compiler, and cmake tools
for compiling/linking to DDS and IDL-derived files.  It's guiding 
principle is that *simple things should be easy*.  So it trys to
adopt sensible defaults while providing access to more functionality
through optional arguments.

Here's the basic "hello world" example from [Fast DDS](https://fast-dds.docs.eprosima.com/en/latest/fastdds/getting_started/simple_app/simple_app.html) 
using the Let's Talk API:

```c++
#include "LetsTalk.hpp"
#include <iostream>
#include "HelloWorld.h"

int main(int, char**)
{
    auto node = lt::Participant::create();
    node->subscribe<HelloWorld>("HelloWorldTopic", [](std::unique_ptr<HelloWorld> data) {
        std::cout << data->message() << " " << data->index() << std::endl;
    }); 
    std::this_thread::sleep_for(std::chrono::minutes(1));            
    return 0;
}

```
and
```c++
#include "LetsTalk.hpp"
#include <iostream>
#include "HelloWorld.h"

int main(int argc, char** argv)
{
    auto node = lt::Participant::create();
    auto pub = node->advertise<HelloWorld>("HelloWorldTopic");
    while(node->subscriberCount("HelloWorldTopic") == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Publication begins...\n";
    for(int i=0; i<100; i++) {
        auto msg = std::make_unique<HelloWorld>();
        msg->message("Test");
        msg->index(i);
        bool okay = pub.publish(std::move(msg));
        std::cout << "Sent " << i << "  " << (okay? "okay" : "FAILED") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (node->subscriberCount("HelloWorldTopic") == 0 || !okay) {
            break;
        }
    }
    return 0;
}

```
As you can see, subscription involves providing a callback function, typically a
lambda.  Publication uses a Publisher object.  Data is handled via unique_ptrs
to avoid concurrency problems.

The design of the main Participant API is largely based on the 
ignition::transport API, a very convenient ZMQ/protobuf communication
system.  Let's Talk is not ignition::transport compatible, however. 
Basic publish/subscribe should be compatible with other DDS vendors
(RTI Connext, Cyclone, etc.), but DDS has long been infamous for 
poor compatibility between vendors, and Let's Talk doesn't try to 
solve that.


# Communication Patterns

Let's talk offers three communication patters:

* Publish/Subscribe: A loosely coupled pattern based on topic names.
  Publishers send data to all subscribers. Subscribers get data from
  all publishers. Let's Talk is capable of both reliable and best 
  effort connections, but does not provide support for persistence
  between multiple runs of a program.
  
* Request/Response: Also known as remote proceedure call (RPC), each
  request will be served by a responder.  Each service is identified 
  by its service name, the request type, and the reply type. Let's 
  Talk implementation is simple. If there are multiple providers for 
  a service, an error is logged, but no attempt is made to determine
  which provider will handle a given call. Calls use the C++ promise/
  future types, including exception setting on failure.
  
* Reactor: A request/response pattern where requests recieve multiple
  "progress" replies, before finally ending with a reply.  These are
  useful in robotics where calculations or actions take appreciable 
  time during which the requesting process may need to cancel or 
  retask the service provider as the situation changes.

The reactor is likewise bound to Let's Talk, and it is unlikely to become
more standards compliant since there are few reactor implementations in
the wild.

# CMake Support

In addition to the API, Let's Talk provides a simple way to use Fast DDS 
with cmake.  The bundled Fast DDS distribution is complete -- it will 
build alongside Let's Talk without other requirements. Adding Let's Talk
as a subdirectory of your CMake project will provide the targets

* letstalk -- The Let's Talk library and transitive dependencies
* fastrtps -- Just the Fast DDS library

Installing the Let's Talk project provides the CMake config script *TODO*.
Using
```
find_package(LetsTalk)
```
will provide the target "lt::LetsTalk" to link against.

## IDL Support in CMake

Working with IDL in Let's Talk is especially easy. Inspired by the protobuf
CMake support, Let's Talk provides an "IdlTarget.cmake" macro.  Basic operation
is
```
list(APPEND CMAKE_MODULE_PATH [path/to/IdlTarget.cmake])
include(IdlTarget)
IdlTarget(myIdlTarget SOURCE MyIdl.idl MyOtherIdl.idl)
...
target_link_library(myTarget PUBLIC myIdlTarget)
```
That is, IdlTarget creates a cmake target consisting of a library built from the 
provided compiled IDLs, linking transitively to Fast CDR, and providing access
to the header include path as a target property.  The header and source files
are stored in the build directory.  If the IDL is changed, make/ninja will correctly
re-run the IDL compiler, recompile the IDL target, and re-link.  The intention is 
to have machine-generated code segregated from the rest of the codebase.  More options
for controlling the include path are documented in IdlTarget.cmake.

# TODO

1. Version script and proper cmake/install

2. Test custom progress data

3. Update bundled fast dds with cmake fixes and "ignore" feature
