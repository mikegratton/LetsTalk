
file(GLOB source CONFIGURE_DEPENDS "*.cpp" "*.cxx")
add_library(LetsTalk ${source})
file(GLOB headers CONFIGURE_DEPENDS "*.hpp" "*.h")

target_link_libraries(LetsTalk PUBLIC fastrtps)
target_include_directories(LetsTalk
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

export(TARGETS LetsTalk fastrtps fastcdr NAMESPACE lt:: FILE LetsTalkTargets.cmake)
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/LetsTalk)
install(TARGETS LetsTalk EXPORT LetsTalk-targets)
install(EXPORT LetsTalk-targets DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/LetsTalk)
