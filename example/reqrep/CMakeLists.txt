IdlTarget(DivideServiceIdl SOURCE DivideService.idl)
add_executable(DivideService DivideService.cpp)
target_link_libraries(DivideService DivideServiceIdl lt::LetsTalk)
add_executable(DivideRequester DivideRequester.cpp)
target_link_libraries(DivideRequester DivideServiceIdl lt::LetsTalk)