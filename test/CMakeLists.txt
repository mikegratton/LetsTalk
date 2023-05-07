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
    reqrep.cpp
    Reactor.cpp
    Guid.cpp
    main.cpp
)
target_link_libraries(ltTest PUBLIC letstalk testIdl)
