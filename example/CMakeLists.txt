cmake_minimum_required(VERSION 3.16)
project(letstalkdemo VERSION 1.0.0)

# Finding our installed xxx-config.cmake modules
list(APPEND CMAKE_PREFIX_PATH 
    ${CMAKE_CURRENT_SOURCE_DIR}/install
)
find_package(LetsTalk)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_subdirectory(pubsub)
add_subdirectory(fastdds)
add_subdirectory(reqrep)
add_subdirectory(reactor)
add_subdirectory(waitset)
