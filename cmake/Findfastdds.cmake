# This is a specialized find script for the fastdds lib
# built and installed by letstalk

find_library(fastcdr_LIB NAME fastcdr
    PATHS
    ${CMAKE_CURRENT_LIST_DIR}/../external
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../
)
find_path(fastcdr_INCLUDE_DIR NAMES LetsTalk/LetsTalk.hpp PATHS ${CMAKE_CURRENT_LIST_DIR}/../../../include)
find_path(fastcdr_LIB_DIR NAMES libfastcdr.so libfastcdr.a PATHS ${CMAKE_CURRENT_LIST_DIR}/../../)

if(fastcdr_LIB AND fastcdr_INCLUDE_DIR AND fastcdr_LIB_DIR)
    set(fastcdr_FOUND true)
endif()

mark_as_advanced(fastcdr_LIB fastcdr_INCLUDE_DIR fastcdr_LIB_DIR)

if(NOT TARGET fastcdr)
    add_library(fastcdr UNKNOWN IMPORTED GLOBAL)
    set_target_properties(fastcdr PROPERTIES IMPORTED_LOCATION ${fastcdr_LIB})
    target_include_directories(fastcdr
        INTERFACE
        ${fastcdr_INCLUDE_DIR}
    )
    target_link_directories(fastcdr INTERFACE ${fastcdr_LIB_DIR})    
endif()


find_library(fastdds_LIB NAMES fastdds
    PATHS
    ${CMAKE_CURRENT_LIST_DIR}/../external
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../
)
find_path(fastdds_INCLUDE_DIR NAMES LetsTalk/LetsTalk.hpp PATHS ${CMAKE_CURRENT_LIST_DIR}/../../../include)
find_path(fastdds_LIB_DIR NAMES libfastdds.so libfastdds.a PATHS ${CMAKE_CURRENT_LIST_DIR}/../../)

if(fastdds_LIB AND fastdds_INCLUDE_DIR AND fastdds_LIB_DIR)
    set(fastdds_FOUND true)
endif()

mark_as_advanced(fastdds_LIB fastdds_INCLUDE_DIR fastdds_LIB_DIR)

if(NOT TARGET fastdds)
    add_library(fastdds UNKNOWN IMPORTED GLOBAL)
    set_target_properties(fastdds PROPERTIES IMPORTED_LOCATION ${fastdds_LIB})
    target_include_directories(fastdds
        INTERFACE
        ${fastdds_INCLUDE_DIR}
    )
    target_link_directories(fastdds INTERFACE ${fastdds_LIB_DIR})
    target_link_libraries(fastdds
        INTERFACE        
        fastcdr
        tinyxml2
        foonathan_memory-0.7.3
    )
endif()