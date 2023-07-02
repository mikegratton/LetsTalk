Let's Talk: A C++ Interprocess Communication System Based on FastDDS
=================================================

# Introduction

Let's Talk is a C++ communication library compatible with DDS (the Data Distribution Service) 
designed for simple and efficient interprocess coordination on local networks. The library is 
simple API for publish/subscribe, request/reply, and reactor communication patterns, along with 
some concurrency tools. Let's Talk also ships a self-contained distribution of FastDDS designed to be 
built inline with you project. CMake support for DDS is provided through macros designed to make
handling IDL files as painless as possible.  It's guiding principle is that 
*simple things should be easy*.  It tries to adopt sensible defaults while providing 
access to more functionality through optional arguments.

# Links

* [GitHub Repository](https://github.com/mikegratton/LetsTalk)
* [Documentation](https://mikegratton.github.io/LetsTalk/)

# Example

Here's the basic "hello world" example from [Fast DDS](https://fast-dds.docs.eprosima.com/en/latest/fastdds/getting_started/simple_app/simple_app.html) 
using the Let's Talk API. The subscriber application:

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
and the publisher application:
```c++
#include "LetsTalk.hpp"
#include <iostream>
#include "HelloWorld.h"

int main(int argc, char** argv)
{
    auto node = lt::Participant::create();
    auto pub = node->advertise<HelloWorld>("HelloWorldTopic");
    while(pub.subscriberCount("HelloWorldTopic") == 0) {
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
Subscription involves little more than providing a callback function (typically a lambda) and a 
topic name.  Publication involves creating a `Publisher` object, then calling `publish()` to send
messages. The publish/subscribe coding is simple and thread-safe; no byzantine class hierarchies, 
obscure quality of service settings, nothing.

The design of the main Participant API is based on the ignition::transport API (but Let's Talk is 
not ignition::transport compatible). Publish/subscribe is compatible with other DDS vendors
(RTI Connext, Cyclone, etc.) to the extent that FastDDS is compatible. The request/reply and reactor
pattern are not compatible outside of Let's Talk. 


# Installation

## As a submodule
The easiest way to use Let's Talk is as a git submodule. You can add it as a submodule via
```
git submodule add -b <desired version branch> git@github.com:mikegratton/LetsTalk.git
```
This will create the directory LetsTalk. In your CMakeLists.txt, add
```
add_subdirectory(LetsTalk)
```
This will provide the following cmake targets:
 
 * `letstalk` -- The library (with appropriate includes)
 * `fastrtps` -- The underlying FastDDS library 
 
To include and link `myTarget` to Let's Talk, you just need to add the cmake
```
target_link_libraries(myTarget PUBLIC letstalk)
```
(The `letstalk` target depends on `fastrtps`, so you don't need to reference `fastrtps` directly.)

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
cmake support, Let's Talk provides an "IdlTarget.cmake" macro.  Basic operation
is
```
list(APPEND CMAKE_MODULE_PATH <your install dir>/lib/cmake/letstalk)
include(IdlTarget)
IdlTarget(myIdlTarget SOURCE MyIdl.idl MyOtherIdl.idl)
...
target_link_library(myTarget PUBLIC myIdlTarget)
```
That is, IdlTarget creates a cmake target consisting of a library built from the 
provided compiled IDLs, linking transitively to all required libraries, and providing access
to the header include path as a target property.  The header and source cpp files
are written in the build directory.  If the IDL is changed, make/ninja will correctly
re-run the IDL compiler, recompile generated source, and re-link.  The intention is 
to have machine-generated code segregated from the rest of the code base.  The full
form is 
```
IdlTarget([PATH path] INCLUDE ... SOURCE ...)
```
where

* `PATH` specifies the relative path where the generated code will be placed. This can be used to
   change the include path. Setting `PATH foo/bar` will change `#include "MyIdl.h"` to `#include "foo/bar/MyIdl"`
* `INCLUDE` specifies additional include paths for IDL compilation
* `SOURCE` gives the list of IDL files that comprise the resultant library. These will be used to generate code,
the code compiled, and then linked into the library.


# Communication Patterns

Let's talk offers three communication patters:

* Publish/Subscribe: A loosely coupled pattern based on topic names.
  Publishers send data to all matching topic subscribers. Subscribers 
  get data from all publishers on that topic. Let's Talk is capable of 
  both reliable and best effort connections.
  
* Request/Reply: Also known as remote procedure call (RPC), each
  request will be served by a responder.  Each service is identified 
  by its service name, the request type, and the reply type. Calls use the 
  C++ promise/future types, including setting exceptions on failure.
  
* Reactor: A request/reply pattern where requests receive multiple
  "progress" replies, before finally ending with a final reply.  These
  are useful in robotics where calculations or actions take 
  appreciable time during which the requesting process may need to 
  cancel or re-task the service provider as the situation changes.

See below for more details on each pattern.  The `examples` directory provides
sample code for each pattern.

## Publish/Subscribe

In pub/sub, a group of publishers send data to all subscribers that match on
a topic.  Topics are strings (e.g. "robot.motion.command") combined with a 
type. Publishers are only matched to subscribers if both the topic name and 
topic types match. To subscribe, you register a callback with a participant,
```cpp
lt::ParticipantPtr node = lt::Participant::create();
node->subscribe<MyType>("my.topic", [](MyType const& sample) {
    std::cout << "Got some data!\n";
});
```
or, if you wish to get samples as `unique_ptr`s (because you plan to move them
to another thread),
```cpp
node->subscribe<MyType>("my.topic", [](std::unique_ptr<MyType> sample) {
    std::cout << "Got some data!\n";
});
```
*IMPORTANT:* This callback is run on an internal thread, so long calculations or
waiting for a lock will negatively impact the whole system. If you do need to do
significant processing, Let's Talk provides a `ThreadSafeQueue` class, and a 
convenience subscription mode,
```cpp
lt::QueuePtr<MyType> myTypeQueue = lt::node->subscribe<MyType>("my.topic");
```
Rather than running a callback, you get a pointer to the queue of messages.
The queue then supplies `pop()` to get a sample (with an optional wait time) and 
`popAll()` to get all pending samples,
```cpp
std::unique_ptr<T> pop(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));
Queue popAll(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));
```
where the `Queue` type is a `std::deque<std::unique_ptr<T>>` by default. This puts
you in charge of when to cause your thread to wait for data. 

If you want to service multiple queues from one thread, you can use the `Waitset` class.
First, register all the queues with the waitset in the constructor:
```cpp
auto queue1 = node->subscribe<MyType1>("my.topic.1");
auto queue2 = node->subscribe<MyType2>("my.topic.2");
Waitset waitset{queue1, queue2};
```
Then, you may `wait` for data to arrive in any of the queue. This blocks the calling 
thread and returns the index of the first queue with pending messages:
```cpp
int triggerIndex = waitset.wait();
switch (triggerIndex) {
    case 0: {
        auto content = queue1->popAll();
        for (auto const& item : content) {
            // Process the data                    
        }
    }
    // Note fallthrough
    case 1: {
        auto content = queue2->popAll();
        for (auto const& item : content) {
            // Process the data
        }
    }
}
```
See `example/waitset` for a detailed design. Note that the returned index represents the first
queue that has data. Higher-numbered queues may also have data. The best practice is to use a 
fall-through design to check all of the higher indexed queues as well.

To cancel a subscription, the Participant provides an unsubscribe function,
```cpp
node->unsubscribe("my.topic");
```
You can also query how many publishers have been discovered for the topic,
```cpp
int count = node->publisherCount("my.topic");
```

For publishing, Participant acts as a factory for creating lightweight `Publisher`
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
Publisher erases the `MyType` information (i.e. all Publishers are formally the same type), 
but publishing a type other than `MyType` will  result in an error. Calling `pub.topicType()` 
will return the type name as a string. You can check for subscribers using 
```cpp
int count = pub.subscriberCount("my.topic")
```
To stop advertising data, simply dispose of the Publisher object.

Let's Talk also supports different "Quality of Service" (QoS) settings for publishers and
subscribers.  An optional string argument to `advertise()` and `subscribe()` 
gives the name of the QoS profile to use. For example,
```
auto frameQueue = node->subscribe<VideoFrame>("video.stream", "bulk");
```
will set the QoS to the bulk mode. See below for a full description of QoS settings in
Let's Talk.

## Request/Reply

In request/reply, the "replier" provides a service that the "requester" accesses. 
A service is defined by a string name, a request type, and a reply type.

The server side, Let's Talk provides two forms: a "push" form that uses a
callback and a "pull" form giving you more control.

The push form calls a callback on a special worker thread to perform the service work.
Callbacks take the form
```cpp
auto myCallback = [](MyRequestType const& i_request) -> MyReplyType { /* ... */ };
```
and the registration call on a participant pointer `node` looks like
```cpp
node->advertise<MyRequestType, MyReplyType>("my.topic", myCallback);
```
Note that you may throw exceptions in the callback to signal failure. The failure of the 
request (though not the specific error) will be forwarded to the requester. This form is 
intended to be very simple to use, but you surrender control over the threading.

The pull form allows you full control over the threading of the 
service, but at the cost of additional complexity. This form creates a `Replier` object
```cpp
auto replier = node->advertise<MyRequestType, MyReplyType>("my.topic");
```
You can obtain pending sessions from this object
```cpp
auto session = replier->getPendingSession(std::chrono::milliseconds(10));
```
specifying a wait time (10 ms here). The session object has a very simple API:
```cpp
class Session {
public:    
    Req const& request() const { return *m_request; }    
    void reply(Rep const& i_reply);
    void reply(std::unique_ptr<Rep> i_reply);
    void fail();
    bool isAlive() { return m_backend != nullptr; }
};
```
You can inspect the request data, provide a reply (closing the session), signal failure (also closing the 
session), or check if the session is live. If no request arrives in the wait time of `getPendingSession()`,
the returned `Session` object will not be alive. You can also attach replier objects to a `Waitset` to wait on
multiple services in one thread.

The client side also has two options. Simple requests may be made directly on the participant via
```cpp
MyRequestType request;
/* ... fill out request */
std::future<MyReplyType> reply = participant->request<MyRequestType, MyReplyType>("my.topic", request);
```
Making a request returns as `std::future` of the reply type. You may block waiting on that future immediately, 
or wait as much as you can afford and come back to the future later.  
This form is somewhat less efficient on the first call, as all of the discovery occurs while you wait, but the 
back-end is stored in the participant so that subsequent requests will be faster.  You may call `unsubscribe()` 
on the participant to delete the standing request subscriptions if you are finished with a service.

Alternatively, you can first create a Requester object 
```cpp
auto requester = participant->request<MyRequestType,MyReplyType>("my.topic");
```
The requester can be used to make multiple requests, check for connectivity, and check for "impostor" services
(more below). Creating it performs all of the discovery tasks that can be time-consuming. The requester API is 
straightforward:
```cpp
class Requester {
   public:        
    std::future<Rep> request(Req const& i_request);
    bool isConnected() const;
    bool impostorsExist() const;
};
```

Two warnings about request/reply:

1. The service callbacks are allowed to throw exceptions and/or signal failure. While the error message isn't propagated 
to the requester, the `std::future` will throw a `std::runtime_error` when `get()` is called. You should use `try/catch`
if the service you are calling may signal requests as failed.

2. Impostor services may exist. Let's Talk does not prevent more than one service provider of the same name
from existing or forward requests to exactly one provider. It will however warn you if more than one provider
exists for a given service. The Requester and Replier both have the function `impostorsExist()` to check for this 
state of affairs.

## Reactor

The Reactor pattern is similar to the request/reply pattern and uses a pull-style API with session objects, but with more 
features. To provide a reactor service, we first create the server object,
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
If this is true, a request has been received. To service it, we obtain a `Session` instance,
```cpp
auto motionSession = motionServer.getPendingSession();
```
This takes an optional wait time if you want to have a blocking wait for sessions. You may also attach
a `ReactorServer` instance to a `Waitset` just as with queues and Repliers. Like the server object, the 
session is lightweight. The session provides accessors for the request data
```cpp
RequestType const& request = motionSession.request();
```
As you process the request, you can send back progress reports via
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
signaling completion. The `ReactorServer` will automatically send these when you start and finish a session.
Note you may send duplicate progress marks, or even have progress decreasing. To signal failure,
`motionSession.fail()` will dispose of the session, notifying the client. To finish a session, send the reply
```cpp
ReplyType reply;
// ... fill out reply
motionSession.reply(reply);
```
Additionally, the client may cancel a session at any time. You can check if the session has been canceled by
calling `isAlive()`.

The client end is likewise similar to the request client. First, we create a client object on the participant:
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
as sample cmake files.  To build the examples, first build and install Let's Talk, then 
create a symlink to the install directory in the examples directory:
```
$ cd examples
$ ln -s <lets talk install dir> install
$ mkdir build && cd build
$ cmake ..
```

# Environment variables

Let's Talk inspects several environment variables so that programs can easily modify the
behavior at runtime.

* `LT_VERBOSE` -- enables debug print messages about discovery and message passing
* `LT_LOCAL_ONLY` -- prevents discovery from finding participants on another host
* `LT_PROFILE` -- Path to custom QoS profile XML file

To use this on your program `foo`, you can launch foo from the shell like this:
```
$ LT_VERBOSE=1 ./foo
```


# Quality of Service (QoS)

QoS determines the reliability of message passing. Let's Talk defines three levels of service that are
always available:

* "reliable" -- the default. This QoS will resend messages when not acknowledged. Publishers 
will also keep the last published message cached so that late-joining subscribers can be 
immediately sent the last message on a topic upon discovery. 

* "bulk" -- Failed sending attempts are not repeated. No queue of old messages is maintained.
This is intended for streaming data where it is better to wait for the next message than retry sending
old data.

* "stateful" -- Like reliable, but samples are delivered in-order to the subscriber. This is for
topics where samples refer to state provided by previous samples. The request/reply and Reactor patterns
use stateful QoS.

To use a different QoS from "reliable," pass the QoS string name to the `subscribe` or `advertise` method.

In addition, Participants may have QoS profiles. These are used to alter the underlying
protocol from UDP to TCP or something else. Currently only UDP profiles are available in the built-in QoS.

## Using Custom QoS

If you wish to develop your own QoS profiles, see https://fast-dds.docs.eprosima.com/en/latest/fastdds/xml_configuration/xml_configuration.html

When invoking your program, set the environment variable `LT_PROFILE` to the path to your xml.  The
`profile_name` attribute may be used as the optional argument for `subscribe` and `advertise` to
use these QoS settings.

# DDS Concepts in Let's Talk

Let's Talk is based on the DDS is a publish/subscribe messaging system. Here's a small primer on DDS concepts.

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


# About DDS

The Data Distribution Service is an efficient and powerful publish/subscribe framework,
but it is very complicated. It's worth exploring the chain of acronyms that make it up:

* DDS -- Data Distribution Service. This isn't a protocol standard at all, it turns out!
It's an API standard. 

* RTPS -- Real-Time Publish Subscribe. This is the protocol standard. It covers how participants
discover one another, how topics and types are communicated, how data is sent, and how transmission
errors are handled. RTPS uses a peer-to-peer design rather than a central message broker, making it 
more resilient and flexible (but also placing larger burdens on those peers).

* IDL -- Interface Description Language. DDS inherited this from the 90's CORBA technology, and the 
16/32 bit world of the time definitely shows in the language.  IDL is ugly but functional for the 
purpose.  It isn't as fully featured as Google Protocol Buffers, but it is serviceable.

* CDR -- Common Data Representation. This is the serialized format for data transmitted. Typically, 
IDL compilers generate native code from IDL that handles serialization to/deserialization from CDR.
Compared to Google Protocol Buffers, CDR is faster to serialize/deserialize, but larger on the wire.
Given DDS's emphasis on local network communication vs protobuf's focus on internet communication,
these design differences makes sense.

# History

## 0.3

* Added a pull-mode replier to request/reply pattern, and added a convenience `request` method to avoid explicitly handling `Requester` objects.

* Send cancellation messages for the reactor when a client goes away unexpectedly.

* Extended the waitset to allow for waiting on `ReactorServer` sessions and Replier sessions. Renamed the object from `QueueWaitset` to 
  `Waitset`.

## 0.2

* Added waitset for waiting on multiple queues in select()-like manner.

* Overhauled the built-in QoS, fixing the "stateful" profile. Removed no longer needed keys from reactor types.

* Fixed `foonathan::memory` default setting that was causing crashes in examples.

* Changed default participant behavior to ignore messages that originated from the same participant. A subscriber and a publisher on 
 the same topic and from the same participant will no longer interact by default.

## 0.1 

Initial release. Covers all basic functionality.

# Roadmap 

## Future Features

1. Automate FastDDS version upgrade

2. To/from json additions for fastddsgen
