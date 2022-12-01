include(IdlTarget)
IdlTarget(messageIdl SOURCE message.idl PATH idl)
add_executable(ping ping.cpp )
add_executable(pong pong.cpp )

target_link_libraries(ping PUBLIC letstalk messageIdl)
target_link_libraries(pong PUBLIC letstalk messageIdl)
