###########################################################################
# IdlTarget -- make a library target whose source comes from compiling idls
#    
# IdlTarget([SHARED] [PATH path] INCLUDE ... SOURCE ...)
#
# Arguments:
#    SHARED -- build a shared library (default is static library)
#    PATH -- Place the h, cxx files in "path," include for foo.idl will be "path/foo.h"
#    INCLUDE -- list of other directories to include while compiling idls
#    SOURCE -- list of idl files 
#
# IdlTarget uses an IDL compiler to generate the cxx source files in the
# build directory, then creates a library target that encapsulates this 
# source. The include path for the IDL headers is also set
############################################################################

#########################################################################
# CompileIdl(<idllist> [includelist])
# 
# Drives compilaion of idls, sets
#   idl_header -- h files from compilation
#   idl_source -- cxx files from compilation
macro(CompileIdl )
   cmake_parse_arguments(idl "" "PATH" "INCLUDE;SOURCE" ${ARGN})
   foreach( incl ${idl_INCLUDE})
       list(APPEND ddsgen_include -I ${incl})
   endforeach()
   set(idl_ABS_PATH ${CMAKE_CURRENT_BINARY_DIR}/${idl_PATH})
   message("idl_PATH=${idl_PATH}")
   file(MAKE_DIRECTORY ${idl_ABS_PATH})
   set(relpath ${idl_ABS_PATH})
   string(REPLACE "${CMAKE_BINARY_DIR}/" "" relpath ${relpath})
       
   find_program(ddsgen fastddsgen PATHS ${PROJECT_SOURCE_DIR}/fastddsgen)
   
   foreach(idl ${idl_SOURCE})
       get_filename_component(stem ${idl} NAME_WE)
       get_filename_component(idl_abs ${idl} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
       add_custom_command(OUTPUT ${idl_ABS_PATH}/${stem}.idl.d
            COMMAND cpp ${ddsgen_include} -MM ${idl_abs} -o ${idl_ABS_PATH}/${stem}.idl.d
            COMMAND sed -i -e "s,${stem}.o,${relpath}/${stem}.h ${relpath}/${stem}.cxx," ${idl_ABS_PATH}/${stem}.idl.d
            DEPENDS ${idl_abs}
            )
       add_custom_command(OUTPUT ${idl_ABS_PATH}/${stem}.h ${idl_ABS_PATH}/${stem}.cxx
            COMMAND ${ddsgen} -cs -replace ${ddsgen_include} ${idl_abs}
            DEPFILE ${idl_ABS_PATH}/${stem}.idl.d
            DEPENDS ${idl_abs} ${idl_ABS_PATH}/${stem}.idl.d
            WORKING_DIRECTORY ${idl_ABS_PATH}
            COMMENT "Compiling idl ${idl_abs}"
            )
       list(APPEND idl_source ${idl_ABS_PATH}/${stem}.cxx)
   endforeach()
endmacro()

###############################################################################
macro(IdlTarget name)
   cmake_parse_arguments(idl "SHARED" "" "" ${ARGN})
   CompileIdl(${idl_UNPARSED_ARGUMENTS})
   if (idl_SHARED)
       add_library(${name} SHARED ${idl_source})
   else()
       add_library(${name} STATIC ${idl_source})
   endif()
   target_link_libraries(${name} PUBLIC fastrtps)
   target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endmacro()
