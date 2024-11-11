cmake_minimum_required(VERSION 3.16)
project(LetsTalk VERSION 0.4.0.0)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_INSTALL_RPATH $ORIGIN)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)
add_subdirectory(src)

enable_testing()
if (BUILD_TESTING)
    add_subdirectory(test)
endif()

install(
    FILES
    cmake/IdlTarget.cmake
    cmake/Findfastdds.cmake
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${LetsTalk_VERSION_MAJOR}.${LetsTalk_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

install(FILES   
    ${PROJECT_SOURCE_DIR}/resources/JsonSupportHeader.stg
    ${PROJECT_SOURCE_DIR}/resources/JsonSupportSource.stg
    DESTINATION share/${PROJECT_NAME}
)

# Some fastdds files end up in the wrong place because they assume
# CMAKE_PROJECT_NAME will be fastdds
install(CODE 
    "file(COPY ${CMAKE_INSTALL_PREFIX}/share/LetsTalk/cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/fastdds/ PATTERN *.cmake ) 
     file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/share/LetsTalk/cmake)"
)
