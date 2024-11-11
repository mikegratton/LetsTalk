# FastDDS CMake has been modified so that these are not searched for. Instead, we include them here
add_subdirectory(memory)
add_subdirectory(fastcdr)

# Configure fastdds to use third party code that is symlinked to its thirdparty/ directory
set(THIRDPARTY FORCE CACHE STRING "FastDDS will use superbuild dependencies" FORCE)
set(THIRDPARTY_UPDATE OFF CACHE BOOL "Attempt to download dependency updates (not recommended)" FORCE)
add_subdirectory(fastdds)

# Include our version of fastddsgen
add_subdirectory(fastddsgen)
