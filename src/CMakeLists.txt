include(IdlTarget)
IdlTarget(LetsTalkIdl PATH idl SOURCE LetsTalk/Reactor.idl)

file(GLOB source CONFIGURE_DEPENDS "LetsTalk/*.cpp" "LetsTalk/*.cxx")
add_library(LetsTalk ${source})

target_link_libraries(LetsTalk PUBLIC LetsTalkIdl fastdds)
target_include_directories(LetsTalk
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
export(TARGETS LetsTalk LetsTalkIdl fastdds fastcdr foonathan_memory NAMESPACE lt:: FILE LetsTalkTargets.cmake)

file(GLOB headers CONFIGURE_DEPENDS "LetsTalk/*.hpp" "LetsTalk/*.h")
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/LetsTalk)
install(TARGETS LetsTalk EXPORT LetsTalk-targets)
# install(EXPORT LetsTalk-targets DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/LetsTalk)
