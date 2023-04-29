include(IdlTarget)
IdlTarget(messageIdl SOURCE message.idl PATH idl)
IdlTarget(helloIdl SOURCE HelloWorld.idl)

add_executable(ltTest
    ActiveObject.cpp
    ThreadSafeQueue.cpp
    PubSub.cpp
    reqrep.cpp
    Reactor.cpp
    Guid.cpp
    main.cpp
)
target_link_libraries(ltTest PUBLIC letstalk messageIdl helloIdl)
