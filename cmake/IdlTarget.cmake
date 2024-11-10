###########################################################################
# IdlTarget -- make a library target whose source comes from compiling idls
#    
# IdlTarget([SHARED] [JSON] [PATH path] INCLUDE ... SOURCE ...)
#
# Arguments:
#    SHARED -- Make the target lib shared, overriding the global setting
#    JSON -- Build the json support extensions (FooJsonSupport.h)
#    PATH -- Place the h, cxx files in "path," include for foo.idl will be "path/foo.h"
#    INCLUDE -- list of other directories to include while compiling idls
#    SOURCE -- list of idl files 
#
# IdlTarget uses an IDL compiler to generate the cxx source files in the
# build directory, then creates a library target that encapsulates this 
# source. The include path for the IDL headers is also set
############################################################################

# ########################################################################
# CompileIdl(<idllist> [includelist])
#
# Drives compilaion of idls, sets
# idl_header -- h files from compilation
# idl_source -- cxx files from compilation
macro(CompileIdl)

cmake_parse_arguments(idl "JSON;SHARED" "PATH" "INCLUDE;SOURCE" ${ARGN})
foreach(incl ${idl_INCLUDE})
    list(APPEND ddsgen_include -I ${incl})
endforeach()
set(idl_ABS_PATH ${CMAKE_CURRENT_BINARY_DIR}/${idl_PATH})
file(TO_CMAKE_PATH ${idl_ABS_PATH} idl_ABS_PATH)
file(MAKE_DIRECTORY ${idl_ABS_PATH})
set(relpath ${idl_ABS_PATH})
string(REPLACE "${CMAKE_BINARY_DIR}/" "" relpath ${relpath})

find_program(ddsgen fastddsgen
    PATHS
        ${PROJECT_SOURCE_DIR}/fastddsgen
        ${PROJECT_SOURCE_DIR}/external/fastddsgen
        ${CompileIdl_location}/../../../bin
        ${CompileIdl_location}/../../bin
        ${CompileIdl_location}/../bin
        /usr/local/bin
        /usr/bin
    )
find_path(stgpath JsonSupportHeader.stg 
    PATHS
        ${PROJECT_SOURCE_DIR}/resources
        ${PROJECT_SOURCE_DIR}/external/fastddsgen
        ${CompileIdl_location}/../../../share/LetsTalk
        ${CompileIdl_location}/../../share/LetsTalk
        ${CompileIdl_location}/../share/LetsTalk
        /usr/local/share/LetsTalk
        /usr/share/LetsTalk    
)

set(idl_options -no-typeobjectsupport -cs -replace -t ${CMAKE_CURRENT_BINARY_DIR}/fastdds)

foreach(idl ${idl_SOURCE})
    get_filename_component(ddsgen_dir ${ddsgen} DIRECTORY)
    get_filename_component(stem ${idl} NAME_WE)
    get_filename_component(idl_abs ${idl} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(idl_output ${idl_ABS_PATH}/${stem}.hpp ${idl_ABS_PATH}/${stem}CdrAux.cxx)
    if (idl_JSON)
        set(json_source ${stem}JsonSupport.cxx)
        set(json_header ${stem}JsonSupport.hpp)
        set(idl_json_options  -extrastg ${stgpath}/JsonSupportHeader.stg ${json_header} -extrastg ${stgpath}/JsonSupportSource.stg ${json_source})        
        list(APPEND idl_output ${idl_ABS_PATH}/${json_source})
    endif()
    add_custom_command(OUTPUT ${idl_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fastdds
        COMMAND ${ddsgen} -d ${idl_ABS_PATH} ${idl_options} ${idl_json_options} ${ddsgen_include} ${idl_abs}
        COMMAND ${CMAKE_COMMAND} -E rename ${idl_ABS_PATH}/${stem}CdrAux.ipp ${idl_ABS_PATH}/${stem}CdrAux.cxx
        DEPENDS ${idl_abs}
        COMMENT " Compiling idl ${idl_abs}"
    )
    list(APPEND idl_source ${idl_output})    
endforeach()
endmacro()

###############################################################################
macro(IdlTarget name)
CompileIdl(${ARGN})
if (idl_SHARED)
    add_library(${name} SHARED ${idl_source})
else()
    add_library(${name} ${idl_source})
endif()
target_link_libraries(${name} PUBLIC fastcdr)
target_include_directories(${name}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/${name}>
)
# If there are no cpp files, we need to ensure that the dummy library has a language
set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)
endmacro()

set(CompileIdl_location ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")
