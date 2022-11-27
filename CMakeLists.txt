cmake_minimum_required(VERSION 3.16)
project (letstalk VERSION 0.1.0.0)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
add_subdirectory(fastdds)
add_subdirectory(src)
add_subdirectory(test)

