include(IdlTarget)
IdlTarget(reactorIdl SOURCE Reactor.idl)

add_library(letstalk
    LetsTalk.cpp
    LetsTalkDetail.cpp
    defaultProfile.cpp
    ActiveObject.cpp
    Reactor.cpp
    Guid.cpp
    ParticipantLogger.cpp
)

set(headers
    FastDdsAlias.hpp
    ActiveObject.hpp
    Guid.hpp
    ReaderListener.hpp
    ReaderListenerImpl.hpp
    ParticipantLogger.hpp
    LetsTalk.hpp
    LetsTalkDetail.hpp
    LetsTalkFwd.hpp
    PubSubType.hpp
    ThreadSafeQueue.hpp
    RequestReplyImpl.hpp
    Reactor.hpp
    ReactorClient.hpp
    ReactorClientImpl.hpp
    ReactorServer.hpp
    ReactorServerImpl.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/Reactor.h
)

target_link_libraries(letstalk PUBLIC fastrtps reactorIdl)
target_include_directories(letstalk
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

export(TARGETS letstalk reactorIdl NAMESPACE lt:: FILE letstalkTargets.cmake)
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/letstalk)
install(TARGETS letstalk reactorIdl EXPORT letstalk-targets)
install(EXPORT letstalk-targets DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/letstalk)
