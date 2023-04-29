# This is a specialized find script for the fastrtps lib
# built and installed by letstalk

find_library(fastrtps_LIB NAMES fastrtps PATHS ${CMAKE_CURRENT_LIST_DIR}/../ ${CMAKE_CURRENT_LIST_DIR}/../../)
find_path(fastrtps_INCLUDE_DIR NAMES letstalk/LetsTalk.hpp PATHS ${CMAKE_CURRENT_LIST_DIR}/../../../include)
find_path(fastrtps_LIB_DIR NAMES libfastrtps.so libfastrtps.a PATHS ${CMAKE_CURRENT_LIST_DIR}/../../)

if(fastrtps_LIB AND fastrtps_INCLUDE_DIR AND fastrtps_LIB_DIR)
    set(fastrtps_FOUND true)
endif()

mark_as_advanced(fastrtps_LIB fastrtps_INCLUDE_DIR fastrtps_LIB_DIR)

if(NOT TARGET fastrtps)
    add_library(fastrtps UNKNOWN IMPORTED GLOBAL)
    set_target_properties(fastrtps PROPERTIES IMPORTED_LOCATION ${fastrtps_LIB})
    target_include_directories(fastrtps
        INTERFACE
        ${fastrtps_INCLUDE_DIR}
    )
    target_link_directories(fastrtps INTERFACE ${fastrtps_LIB_DIR})
    target_link_libraries(fastrtps
        INTERFACE
        fastrtps
        fastcdr
        tinyxml2
        foonathan_memory-0.7.2
    )
endif()