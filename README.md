Let's Talk: A C++ Interprocess Communication System Based on FastDDS
=================================================

# Introduction

Let's Talk is a C++ communication library compatible with DDS (the Data Distribution Service) 
designed for simple and efficient interprocess coordination on local networks. As DDS is
the communication standard used by ROS2, it's compatible with ROS2. The library is 
an API wrapped around the FastDDS library, along with a distribution of that library
and cmake tools for compiling/linking to DDS.  It's guiding principle is that 
*simple things should be easy*.  So it trys to adopt sensible defaults while providing 
access to more functionality through optional arguments.

# Links

* [GitHub Repository](https://github.com/mikegratton/LetsTalk)
* [Documentation](https://mikegratton.github.io/LetsTalk/)

# Example

Here's the basic "hello world" example from [Fast DDS](https://fast-dds.docs.eprosima.com/en/latest/fastdds/getting_started/simple_app/simple_app.html) 
using the Let's Talk API:

```c++
#include "LetsTalk.hpp"
#include <iostream>
#include "HelloWorld.h"

int main(int, char**)
{
    auto node = lt::Participant::create();
    node->subscribe<HelloWorld>("HelloWorldTopic", [](HelloWorld const& data) {
        std::cout << data.message() << " " << data.index() << std::endl;
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
        HelloWorld msg;
        msg.message("Test");
        msg.index(i);
        bool okay = pub.publish(msg);
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
lambda.  Publication uses a Publisher object.

The design of the main Participant API is largely based on the 
ignition::transport API, a very convenient ZMQ/protobuf communication
system.  Let's Talk is not ignition::transport compatible, however. 
Basic publish/subscribe should be compatible with other DDS vendors
(RTI Connext, Cyclone, etc.), but DDS has long been infamous for 
poor compatibility between vendors, and Let's Talk doesn't try to 
solve that.


# Installation

## As a submodule
The easiest way to use Let's Talk is as a git submodule.
```
git submodule add -b <desired version branch> git@github.com:mikegratton/LetsTalk.git
```
In your CMakeLists.txt, add
```
add_subdirectory(LetsTalk)
```
This will provide the following cmake targets:
 
 * letstalk -- The library (with appropriate includes)
 * fastrtps -- The underlying FastDDS library 
 
 To include and link `myTarget` to letstalk, you just need to add the CMake
 ```
 target_link_libraries(myTarget PUBLIC letstalk)
 ```
 (Letstalk depends on fastrtps, but you don't need to reference it directly.)

 ## Via an installation
 If you have several projects that depend on Let's Talk, it is more efficient
 to install the library per usual. In this case, check out the code and do
 ```
 mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=<your install dir> -DCMAKE_BUILD_TYPE=Release && make install
 ```
 This will provide a cmake config script at `<your install dir>/lib/cmake/letstalk` that you can use 
 in your cmake like
 ```
 list(APPEND CMAKE_MODULE_PATH <your install dir>/lib/cmake/letstalk)
list(APPEND CMAKE_PREFIX_PATH <your install dir>/lib/cmake/letstalk)
 find_package(letstalk)
 ```
 This will provide the same cmake targets (`letstalk` and `fastrtps`) for linking as 
 above.

## IDL Support in CMake

Working with IDL in Let's Talk is especially easy. Inspired by the protobuf
CMake support, Let's Talk provides an "IdlTarget.cmake" macro.  Basic operation
is
```
list(APPEND CMAKE_MODULE_PATH <your install dir>/lib/cmake/letstalk)
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



# Communication Patterns

Let's talk offers three communication patters:

* Publish/Subscribe: A loosely coupled pattern based on topic names.
  Publishers send data to all subscribers. Subscribers get data from
  all publishers. Let's Talk is capable of both reliable and best 
  effort connections, but does not provide support for persistence
  between multiple runs of a program.
  
* Request/Reply: Also known as remote proceedure call (RPC), each
  request will be served by a responder.  Each service is identified 
  by its service name, the request type, and the reply type. Let's 
  Talk implementation is simple. If there are multiple providers for 
  a service, an error is logged, but no attempt is made to determine
  which provider will handle a given call. Calls use the C++ promise/
  future types, including setting exceptions on failure.
  
* Reactor: A request/reply pattern where requests recieve multiple
  "progress" replies, before finally ending with a final reply.  These
  are useful in robotics where calculations or actions take 
  appreciable time during which the requesting process may need to 
  cancel or retask the service provider as the situation changes.

Below are some more details on each.  See also the `examples` directory with
sample code to crib from.

## Publish/Subscribe

In pub/sub, a group of publishers send data to all subscribers that match on
a topic.  Topics are strings (e.g. "robot.motion.command") but also have a 
defined type.  To subscribe, you register a callback with a participant,
```cpp
lt::ParticipantPtr node = lt::Participant::create();
node->subscribe<MyType>("my.topic", [](MyType const& sample) {
    std::cout << "Got some data!\n";
});
```
or, if you wish to get samples as unique_ptrs (because you plan to move them
to another thread),
```cpp
node->subscribe<MyType>("my.topic", [](std::unique_ptr<MyType> sample) {
    std::cout << "Got some data!\n";
});
```
*IMPORTANT:* This callback is run on a pub/sub thread, so long calculations or
waits for a lock will negatively impact the whole system. A good practice is to 
simply enqueue the data on a thread-safe queue for later processing. Let's Talk
provides such a queue as `ThreadSafeQueue`, and a convenience subscription mode,
```cpp
lt::QueuePtr<MyType> myTypeQueue = lt::node->subscribe<MyType>("my.topic");
```
The queue then supplies `pop()` to get a sample (with an optional wait time) and 
`popAll()` to get all pending samples,
```cpp
std::unique_ptr<T> pop(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));
Queue popAll(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));
```
where the `Queue` type is a `std::deque<std::unique_ptr<T>>` by default. 

You can use the `QueueWaitset` to wait on multiple queues in a `select`-like manner.
First, register all the queues with the waitset at construction:
```cpp
auto queue1 = node->subscribe<MyType1>("my.topic.1");
auto queue2 = node->subscribe<MyType2>("my.topic.2");
auto waitset = makeQueueWaitset(queue1, queue2);
```
Later, you may `wait` for data, blocking the calling thread and returning the index
of the first queue with pending messages
```cpp
int triggerIndex = waitset.wait();
switch (triggerIndex) {
    case 0: {
        auto content = queue1->popAll();
        if (queue1.size() > 0) {
            // Process the data                    
        }
    }
    // Note fallthrough
    case 1: {
        auto content = queue2->popAll();
        if (queue2.size() > 0) {
            // Process the data
        }
    }
}
```
See `example/waitset`

To cancel a subscription, the Participant provides an unsubscribe function,
```cpp
node->unsubscribe("my.topic");
```
You can also query how many publishers have been discovered for the topic,
```cpp
int count = node->publisherCount("my.topic");
```

On the publisher end, Participant acts as a factory for creating lightweight `Publisher`
objects,
```cpp
lt::ParticipantPtr node = lt::Participant::create();
lt::Publisher pub = node->advertise<MyType>("my.topic");
```
To use it,
```cpp
MyType sample;
/* ... fill out sample here */
pub.publish(sample);
```
or via unique_ptr,
```cpp
auto sample = std::make_unique<MyType>();
/* ... fill out sample here */
pub.publish(std::move(sample));
```
Publisher erases the `MyType` information, but publishing a type other than `MyType` will 
result in an error. Calling `pub.topicType()` will return the typename as a string. 
You can check for subscribers using the Participant,
```cpp
int count = node->subscriberCount("my.topic")
```
To stop advertising data, simply dispose of the Publisher object.

Let's Talk also supports differen "Quality of Service" (QoS) settings for publishers and
subscribers.  An optional string argument to `advertise()` and `subscribe()` 
gives the name of the QoS profile to use. For example,
```
auto frameQueue = node->subscribe<VideoFrame>("video.stream", "bulk");
```
will set the QoS to the bulk mode. See below for a full description of QoS settings in
Let's Talk.

## Request/Reply

In request/reply, a service is defined by a string name, a request type, and a reply type.
Let's Talk automatically generates topics for the request and reply from this information. The
server side, the one providing the service, registers a callback to perform the service work.
Callbacks take the form
```cpp
auto myCallback = [](MyRequestType const& i_request) -> MyReplyType { /* ... */ };
```
and the registration call on a participant pointer `node` looks like
```cpp
node->advertise<MyRequestType, MyReplyType>("my.topic", myCallback);
```
A new thread is spawned for running the callback, so efficiency in the callback is not essential.

To make a request from another participant, first create a Requester object on that node:
```cpp
auto requester = participant->request<MyRequestType,MyReplyType>("my.topic");
```
The requester can be used to make multiple requests. Creating it performs all of the discovery tasks
that can be time-consuming. The requester API is straightforward. Making a request returns as
`std::future` of the reply type. You may block waiting on that future immediately, like so
```cpp
MyRequestType request;
/* ... fill out request */
auto reply = requester.request(request).get();
```
or wait as much as you can afford and come back to the future later.  The requester also has
the `isConnected()` method to check if the server has been found.

Two warnings about request/reply:

1. The service callbacks are allowed to throw exceptions. While the error message isn't propigated to the 
requester, the `std::future` will throw a `std::runtime_error` when `get()` is called. You should use `try/catch`
if the service you are calling will throw.

2. Impostor services may exist. Let's Talk does not prevent more than one service provider of the same name
from existing or forward requests to exactly one provider. It will however warn you if more than one provider
exists for a given service. The Requester has a function call `impostorsExist()` to check for this state of 
affairs.

## Reactor

The Reactor uses pull-style API with session objects rather than callbacks. The server-side
differs from the request/reply. To provide a reactor service, we first create the server
object,
```cpp
lt::ParticipantPtr node = lt::Participant::create();
auto motionServer = node->makeReactorServer<RequestType, ReplyType, ProgressType>("robot.move");
```
Here, `ProgressType` is optional; if your service doesn't provide progress data you can omit the
argument. `ReactorServer` objects are lightweight and may be copied cheaply. To see if clients have connected,
```cpp
int knownClientCount = motionServer.discoveredClients());
```
and to check for pending sessions,
```cpp
bool pending = motionServer.havePendingSession();
```
If this is true, a request has been recieved. To service it, we get a `Session` object,
```cpp
auto motionSession = motionServer.getPendingSession();
```
This takes an optional wait time if you want to have a blocking wait for sessions. Like the 
server object, this is a lightweight object that may be copied cheaply. Copies all refer to the 
same logical session. The session object provides accessors for the request data
```cpp
RequestType const& request = motionSession.request();
```
We can begin processing the request now.  As we go, we can send back progress reports via
```cpp
ProgressData mySpecialProgress;
// ... fill out data
motionSession.progress(25, mySpecialProgress);
```
If you didn't specify a `ProgressData` type, you may still send progress marks with
```
motionSession.progress(25);
```
The progress mark integer uses values from 1 to 100, with 1 being a special value for "started" and 100
signaling completion. The ReactorServer will automatically send these when you start and finish a session.
Note you may send duplicate progress marks, or even have progress decreasing. To signal failure,
`motionSession.fail()` will dispose of the session, notifying the client. To finish a session,
```cpp
ReplyType reply;
// ... fill out reply
motionSession.reply(reply);
```
Additionally, the client may cancel a session at any time. You can check if the session has been cancelled by
calling `isAlive()`.

The client end is similar to the request/reply client. First, we create a client object on the participant:
```cpp
auto motionClient = node->makeReactorClient<RequestType, ReplyType, ProgressType>("robot.motion");
```
We can then check for connections with `motionClient.discoveredServer()`. Sending a request starts a session
```cpp
RequestData request;
// ... fill out request
auto clientSession = motionClient.request(request);
```
The session API provides calls to determine if the session is alive (started by the server), get the current 
progress, get a progress data sample, or await the final reply.  There's also a `cancel()` call to end the 
session early.

## Examples

The `examples` directory contains demonstration programs for these three patterns, as well
as sample CMake files.  To build the examples, first build and install Let's Talk, then 
create a symlink to the install directory in the examples directory:
```
$ cd examples
$ ln -s <lets talk install dir> install
$ mkdir build && cd build
$ cmake ..
```

# Quality of Service (QoS)

Let's Talk defines three levels of service by default:

* "reliable" -- the default. This QoS will resend messages when not acknowledged. Publishers 
will also keep the last published message cached so that late-joining subscribers can be 
immediately sent the last message on a topic upon discovery. 

* "bulk" -- Failed sending attempts are not repeated. No queue of old messages is maintained.
This is intended for streaming data where it is better to press ahead than dwell upon the past.

* "stateful" -- Like reliable, but samples are delivered in-order to the subscriber. This is for
topics where samples refer to state provided by previous samples. Note that FastDDS doesn't yet
support this, so "stateful" behaves exactly as "reliable" for now.

In addition, Participants may have QoS profiles. These are used to alter the underlying
protocol from UDP to TCP or something else. Currently only UDP profiles are defined by default.

## Using Custom QoS

If you wish to develop your own QoS profiles, see https://fast-dds.docs.eprosima.com/en/latest/fastdds/xml_configuration/xml_configuration.html

When invoking your program, set the enivronment variable `LT_PROFILE` to the path to your xml.  The
`profile_name` attribute may be used as the optional argument for `subscribe` and `advertise` to
use these QoS settings.

# DDS Concepts in Let's Talk

The DDS is a publish/subscribe messaging system with automatic discovery. Here's a small primer.

## Participant

Each node in the DDS network is a "participant."  Participants discover each other, trading information
on available topics and types. Participants also function as factories for the other objects -- topic
objects, types, publishers, and subscribers.

## Topics

A topic is a channel for data. It's the combination of a string topic name and a data type. The types 
generally must be derived from IDL.

## Types and IDL

Types in DDS are typically derived from IDL source. IDL provides a C-like language for describing structured 
data. The IDL compiler produces C++ source code from these files that includes serialization and deserialization
methods to/from the Common Data Format (CDR). DDS automatically performs the required serialization and
deserialization as required.

## Quality of Service (QoS)

An overloaded term, QoS refers to all of the run-time settings available in DDS. It includes the network
protocol (TCP, UDP, shared memory), the error handling strategy, the depth of message history that is stored,
and many other details.

# Environment variables

Let's Talk uses environment variables so that programs can easily modify the
behavior at runtime.

* `LT_VERBOSE` -- enables debug print messages about discovery and message passing
* `LT_LOCAL_ONLY` -- If 1, prevents discovery from finding participants on another host
* `LT_PROFILE` -- Path to custom QoS profile XML file

To use this on you program `foo`, you can launch foo from the shell like this:
```
$ LT_VERBOSE=1 ./foo
```


# About DDS

The Data Distribution Service is an efficient and powerful publish/subscribe framework,
but it is very complicated. It's worth exploring the chain of acronyms that make it up:

* DDS -- Data Distribution Service. This isn't a protocol standard at all, it turns out!
It's an API standard. 

* RTPS -- Real-Time Publish Subscribe. This is the protocol standard. It covers how participants
discover one another, how topics and types are communicated, how data is sent, and how transmission
errors are handled. RTPS uses a peer-to-peer design rather than a central message broker, making it 
more resillient and flexible. (But also placing larger burdens on those peers.)

* IDL -- Interface Description Language. DDS inherited this from the 90's CORBA technology, and the 
16/32 bit world of the time defintely shows in the language.  IDL is ugly but functional for the 
purpose.  It isn't as fully featured as Google Protocol Buffers, but it is servicable.

* CDR -- Common Data Representation. This is the serialized format for data transmitted. Typically, 
IDL compilers generate native code from IDL that handles serialization to/deserialization from CDR.
Compared to Google Protocol Buffers, CDR is faster to serialize/deserialize, but larger on the wire.
Given DDS's emphasis on local network communication vs protobuf's focus on internet communication,
this makes sense.

# History


## 0.2

* Added waitset for waiting on multiple queues in select()-like manner.

* Overhalled builtin QoS, fixing the "stateful" profile. Removed no longer needed keys from reactor types.

* Fixed foonathan::memory default setting that was causing crashes in examples.

* Changed default participant behavior to ignore messages that originated from the same participant. A subscriber and a publisher on 
 the same topic and from the same participant will no longer interact by default.

## 0.1 

Initial release. Covers all basic functionality.

# Roadmap 

## Future Features

1. Automate FastDDS version upgrade

2. To/from json additions for fastddsgen

