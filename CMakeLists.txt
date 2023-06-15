cmake_minimum_required(VERSION 3.16)
project(letstalk VERSION 0.2.0)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_INSTALL_RPATH $ORIGIN)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO this is busted
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "install path" FORCE)
endif()

add_subdirectory(fastdds)
add_subdirectory(fastddsgen)
add_subdirectory(src)
add_subdirectory(test)

install(
    FILES
    cmake/IdlTarget.cmake
    cmake/Findfastrtps.cmake
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME})
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${letstalk_VERSION_MAJOR}.${letstalk_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
