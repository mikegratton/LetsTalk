#include "LetsTalk/LetsTalk.hpp"
#include "doctest.h"
#include "idl/HelloWorld.hpp"

TEST_CASE("Waitset.ctor")
{
    auto p = lt::Participant::create(0, "Test");
    auto q1 = p->subscribe<HelloWorld>("hello");
    lt::Waitset waitset{q1};
    auto replier = p->advertise<HelloWorld, HelloWorld>("hello-service");
    waitset.attach(replier);
    auto p2 = lt::Participant::create(0, "Test2");
    auto requester = p2->makeRequester<HelloWorld, HelloWorld>("hello-service");
    HelloWorld req;
    req.index(0);
    req.message("Hello");
    requester.request(req);
    int readyIndex = waitset.wait();
    CHECK(readyIndex == 1);
    replier.getPendingSession().fail();
    auto reactorServer = p->makeReactorServer<HelloWorld, HelloWorld>("hello-reactor");
    waitset.attach(reactorServer);
    auto reactorClient = p2->makeReactorClient<HelloWorld, HelloWorld>("hello-reactor");
    auto clientSession = reactorClient.request(req);
    readyIndex = waitset.wait();
    CHECK(readyIndex == 2);
    auto server = lt::castToReactor<HelloWorld, HelloWorld>(waitset.get(readyIndex));
    CHECK(server.isOkay());
    auto replier2 = lt::castToReplier<HelloWorld, HelloWorld>(waitset.get(1));
    CHECK(replier2.isOkay());
    lt::Waitset waitset2{q1, replier, reactorServer};
}