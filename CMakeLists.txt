cmake_minimum_required(VERSION 3.16)
project (letstalk VERSION 0.1.0.0)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "install path" FORCE)
endif()

add_subdirectory(fastdds)
add_subdirectory(fastddsgen)
add_subdirectory(src)
add_subdirectory(test)


install(FILES cmake/IdlTarget.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/letstalk/cmake)
