add_library(asio INTERFACE)
target_include_directories(asio 
    INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  
    )
    
export(TARGETS asio NAMESPACE asio:: FILE asioTargets.cmake)
install(DIRECTORY include      
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    PATTERN Makefile.* EXCLUDE )
install(TARGETS asio EXPORT asioTargets)
install(EXPORT asioTargets DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/asio)
