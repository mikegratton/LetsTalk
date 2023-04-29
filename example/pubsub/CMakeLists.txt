
include(IdlTarget)
IdlTarget(helloIdl SOURCE HelloWorld.idl)

add_executable(publisher Publisher.cpp)
target_link_libraries(publisher PUBLIC helloIdl letstalk)
add_executable(listener Listener.cpp)
target_link_libraries(listener PUBLIC helloIdl letstalk)
