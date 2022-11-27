###########################################################################
# IdlTarget -- make a library target whose source comes from compiling idls
#    
# IdlTarget([SHARED] INCLUDE ... SOURCE ...)
#
# Arguments:
#    SHARED -- build a shared library (default is static library)
#    INCLUDE -- list of other directories to include while compiling idls
#    SOURCE -- list of idl files 
#
# IdlTarget uses an IDL compiler to generate the cxx source files in the
# build directory, then creates a library target that encapsulates this 
# source. The include path for the IDL headers is also set
# TODO(MBG): Make the location of the headers configurable
############################################################################

#########################################################################
# CompileIdl(<idllist> [includelist])
# 
# Drives compilaion of idls, sets
#   idl_header -- h files from compilation
#   idl_source -- cxx files from compilation
macro(CompileIdl idllist)
   
   foreach(idl ${idllist})
       get_filename_component(stem ${idl} NAME_WE)
       get_filename_component(idl_abs ${idl} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
       list(APPEND idl_header ${stem}.h)
       list(APPEND idl_source ${stem}.cxx)
       list(APPEND idl_abs )
   endforeach()
   
   set(include ${ARGN})
   foreach( incl ${include})
       list(APPEND ddsgen_include -I ${incl})
   endforeach()
   
   find_program(ddsgen fastddsgen PATHS ${PROJECT_SOURCE_DIR}/fastddsgen)
   add_custom_command(OUTPUT ${idl_header} ${idl_source}
       COMMAND ${ddsgen} -cs -replace ${ddsgen_include} ${idl_abs}
       DEPENDS ${idl_abs}
       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   )
endmacro()

###############################################################################
macro(IdlTarget name)
   cmake_parse_arguments(idl "SHARED" "" "INCLUDE;SOURCE" ${ARGN})
   CompileIdl(${idl_SOURCE} ${idl_INCLUDE})
   if (idl_SHARED)
       add_library(${name} SHARED ${idl_source})
   else()
       add_library(${name} STATIC ${idl_source})
   endif()
   target_link_libraries(${name} PUBLIC fastrtps)
   target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endmacro()
