
include(IdlTarget)
IdlTarget(helloIdl SOURCE HelloWorld.idl)

add_executable(publisher Publisher.cpp)
target_link_libraries(publisher PUBLIC helloIdl letstalk)
add_executable(subscriber Subscriber.cpp)
target_link_libraries(subscriber PUBLIC helloIdl letstalk)

add_executable(queueSubscriber QueueSubscriber.cpp)
target_link_libraries(queueSubscriber PUBLIC helloIdl letstalk)
