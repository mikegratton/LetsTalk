include(IdlTarget)
CompileIdl(PATH "LetsTalk" SOURCE LetsTalk/ReactorIdl.idl)
file(GLOB headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/LetsTalk/*.hpp")
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/LetsTalk/)

file(GLOB source CONFIGURE_DEPENDS "LetsTalk/*.cpp")
add_library(LetsTalk ${source} ${idl_source})

target_link_libraries(LetsTalk PUBLIC fastdds)
target_include_directories(LetsTalk
    PRIVATE  
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/LetsTalk>
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

file(GLOB headers CONFIGURE_DEPENDS "LetsTalk/*.hpp")
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/LetsTalk)

install(
    TARGETS 
        LetsTalk         
    EXPORT LetsTalk-targets     
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/lib
)

install(
    EXPORT LetsTalk-targets 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/LetsTalk            
    NAMESPACE lt::
    FILE LetsTalkTargets.cmake
)
