# include(IdlTarget)
# IdlTarget(reactorIdl SOURCE Reactor.idl)

add_library(letstalk
    Participant.cpp
    LetsTalkDetail.cpp
    defaultProfile.cpp
    ActiveObject.cpp
    Reactor.cpp
    Guid.cpp
    ParticipantLogger.cpp
    Reactor.cxx
)

set(headers
    Awaitable.hpp
    LetsTalk.hpp
    FastDdsAlias.hpp
    ActiveObject.hpp
    Guid.hpp
    ReaderListener.hpp
    ReaderListenerImpl.hpp
    LetsTalkFwd.hpp
    PubSubType.hpp
    ThreadSafeQueue.hpp
    Participant.hpp
    ParticipantImpl.hpp
    ParticipantLogger.hpp
    RequestReply.hpp
    RequestReplyImpl.hpp
    Reactor.h
    Reactor.hpp
    ReactorClient.hpp
    ReactorClientImpl.hpp
    ReactorServer.hpp
    ReactorServerImpl.hpp
    Waitset.hpp
    meta.hpp
)

target_link_libraries(letstalk PUBLIC fastrtps)
target_include_directories(letstalk
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

export(TARGETS letstalk NAMESPACE lt:: FILE letstalkTargets.cmake)
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/letstalk)
install(TARGETS letstalk EXPORT letstalk-targets)
install(EXPORT letstalk-targets DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/letstalk)
