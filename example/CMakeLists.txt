cmake_minimum_required(VERSION 3.16)
project(letstalkdemo VERSION 1.0.0)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/install/lib/cmake/letstalk)
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/install/lib/cmake/letstalk)

find_package(letstalk)

add_subdirectory(pubsub)
add_subdirectory(fastdds)
add_subdirectory(reqrep)
add_subdirectory(reactor)
