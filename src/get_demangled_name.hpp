#pragma once
#include <string>
#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>


template<class T>
std::string get_demangled_name()
{
    char* realname = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    std::string rname(realname);
    free(realname);
    return rname;
}
#elif defined(_MSC_VER)

template<class T>
std::string get_demangled_name()
{
    return std::string(typeid(T).name());
}
#endif
