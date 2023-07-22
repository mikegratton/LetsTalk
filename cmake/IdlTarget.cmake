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
set(idl_options -cs;-replace)
if (idl_JSON)
    list(APPEND idl_options -json)
endif()

foreach(idl ${idl_SOURCE})
    get_filename_component(stem ${idl} NAME_WE)
    get_filename_component(idl_abs ${idl} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(idl_output ${idl_ABS_PATH}/${stem}.h;${idl_ABS_PATH}/${stem}.cxx)
    if (idl_JSON)
        list(APPEND idl_output ${idl_ABS_PATH}/${stem}JsonSupport.cxx)
    endif()
    if(CMAKE_GENERATOR STREQUAL " Ninja ")
        add_custom_command(OUTPUT ${idl_output}
            COMMAND cpp ${ddsgen_include} -MM ${idl_abs} -MF ${idl_ABS_PATH}/${stem}.idl.d
            COMMAND sed -i -e " s,${stem}.o,${relpath}/${stem}.h ${relpath}/${stem}.cxx, " ${idl_ABS_PATH}/${stem}.idl.d
            COMMAND ${ddsgen} -d ${idl_ABS_PATH} ${idl_options} ${ddsgen_include} ${idl_abs}
            DEPFILE ${idl_ABS_PATH}/${stem}.idl.d
            DEPENDS ${idl_abs}
            COMMENT " Compiling idl ${idl_abs}"
        )
    else()
        add_custom_command(OUTPUT ${idl_output}
            COMMAND ${ddsgen} -d ${idl_ABS_PATH} ${idl_options} ${ddsgen_include} ${idl_abs}
            DEPENDS ${idl_abs}
            COMMENT " Compiling idl ${idl_abs}"
        )
    endif()
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
target_link_libraries(${name} PUBLIC fastrtps)
if (idl_JSON)
    if (TARGET json)
        target_link_libraries(${name} PRIVATE json)
    else()
        find_path(json_header json.hpp
            PATHS
            ${PROJECT_SOURCE_DIR}/external/json
            ${CompileIdl_location}/../../../include
            ${CompileIdl_location}/../../include
            ${CompileIdl_location}/../include
            /usr/local/include            
            /usr/include
            PATH_SUFFIXES
            letstalk
            nlohmann
        )
        target_include_directories(${name} PRIVATE ${json_header})
    endif()
endif()
target_include_directories(${name}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/${name}>
)
endmacro()

set(CompileIdl_location ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")
mark_as_advanced(CompileIdl_location)
