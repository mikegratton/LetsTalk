add_library(letstalk SHARED LetsTalk.cpp LetsTalkDetail.cpp defaultProfile.cpp)
target_link_libraries(letstalk PUBLIC fastrtps)
target_include_directories(letstalk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(letstalk PUBLIC ${CMAKE_BINARY_DIR}/test)
