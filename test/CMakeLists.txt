include(IdlTarget)
IdlTarget(testIdl
    SOURCE
        message.idl
        other.idl
        HelloWorld.idl
        OpenGarage.idl
    PATH idl)

add_executable(ltTest
    ActiveObject.cpp
    ThreadSafeQueue.cpp
    PubSub.cpp
    ReqRep.cpp
    Reactor.cpp
    Guid.cpp
    KeepAllTest.cpp
    Waitset.cpp
    main.cpp
)
target_link_libraries(ltTest PUBLIC letstalk testIdl)
