add_library(json INTERFACE)
target_include_directories(json INTERFACE ${CMAKE_CURRENT_LIST_DIR})
install(FILES json.hpp DESTINATION ${CMAKE_INSTALL_PREFIX}/include)