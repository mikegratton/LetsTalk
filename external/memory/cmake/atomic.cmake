# check for atomic library, which is needed on some architectures
include(CheckCXXSourceCompiles)
function(check_cxx_atomic_compiles varname)
	check_cxx_source_compiles("
	#include <atomic>
	std::atomic<bool> x;
	int main() {
		bool y = false;
		return !x.compare_exchange_strong(y, true);
	}" ${varname})
endfunction()
function(check_working_cxx_atomic varname)
	check_cxx_atomic_compiles(${varname}_NOFLAG)
	if(${varname}_NOFLAG)
		set(${varname} TRUE PARENT_SCOPE)
		return()
	endif()
	set(old_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
	list(APPEND CMAKE_REQUIRED_FLAGS "-std=c++11")
	check_cxx_atomic_compiles(${varname}_STD11)
	set(CMAKE_REQUIRED_FLAGS  "${old_CMAKE_REQUIRED_FLAGS}")
	if(${varname}_STD11)
		set(${varname} TRUE PARENT_SCOPE)
		return()
	endif()
	list(APPEND CMAKE_REQUIRED_FLAGS "-std=c++0x")
	check_cxx_atomic_compiles(${varname}_STD0X)
	set(CMAKE_REQUIRED_FLAGS  "${old_CMAKE_REQUIRED_FLAGS}")
	if(${varname}_STD0X)
		set(${varname} TRUE PARENT_SCOPE)
		return()
	endif()
	set(${varname} FALSE PARENT_SCOPE)
endfunction()
check_working_cxx_atomic(HAVE_CXX_ATOMIC)
if(NOT HAVE_CXX_ATOMIC)
	set(old_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
	list(APPEND CMAKE_REQUIRED_LIBRARIES "-latomic")
	check_working_cxx_atomic(NEED_LIBRARY_FOR_CXX_ATOMIC)
	set(CMAKE_REQUIRED_LIBRARIES "${old_CMAKE_REQUIRED_LIBRARIES}")
	if(NOT NEED_LIBRARY_FOR_CXX_ATOMIC)
		message(FATAL_ERROR "Host compiler does not support std::atomic")
	endif()
endif()