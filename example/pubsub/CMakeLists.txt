
include(IdlTarget)
IdlTarget(helloIdl JSON SOURCE HelloWorld.idl)
add_executable(publisher Publisher.cpp)
target_link_libraries(publisher PUBLIC helloIdl LetsTalk)
add_executable(subscriber Subscriber.cpp)
target_link_libraries(subscriber PUBLIC helloIdl LetsTalk)

add_executable(queueSubscriber QueueSubscriber.cpp)
target_link_libraries(queueSubscriber PUBLIC helloIdl LetsTalk)
