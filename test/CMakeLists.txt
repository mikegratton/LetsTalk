include(IdlTarget)
IdlTarget(messageIdl SOURCE message.idl PATH idl)
IdlTarget(helloIdl SOURCE HelloWorld.idl)

add_executable(ping ping.cpp )
add_executable(pong pong.cpp)
target_link_libraries(ping PUBLIC letstalk messageIdl helloIdl)
target_link_libraries(pong PUBLIC letstalk messageIdl helloIdl)


add_executable(helloPub HelloWorldPublisher.cpp ${CMAKE_CURRENT_BINARY_DIR}/HelloWorldPubSubTypes.cxx)
add_executable(helloSub HelloWorldSubscriber.cpp ${CMAKE_CURRENT_BINARY_DIR}/HelloWorldPubSubTypes.cxx)
target_link_libraries(helloPub PUBLIC letstalk helloIdl)
target_link_libraries(helloSub PUBLIC letstalk helloIdl)

add_executable(ltTest
    ActiveObject.cpp
    ThreadSafeQueue.cpp
    qosProfile.cpp
    reqrep.cpp
    main.cpp
    )
target_link_libraries(ltTest PUBLIC letstalk helloIdl)
