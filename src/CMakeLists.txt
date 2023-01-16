add_library(letstalk  
    LetsTalk.cpp 
    LetsTalkDetail.cpp 
    defaultProfile.cpp 
    ActiveObject.cpp
    )

set(headers 
    ActiveObject.hpp
    LetsTalk.hpp
    LetsTalkDetail.hpp
    LetsTalkFwd.hpp
    PubSubType.hpp
    ThreadSafeQueue.hpp
    )
    
target_link_libraries(letstalk PUBLIC fastrtps)
target_include_directories(letstalk 
    PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>  
    )

export(TARGETS letstalk NAMESPACE lt:: FILE letstalkTargets.cmake)
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/letstalk)    
install(TARGETS letstalk EXPORT letstalk-targets)
install(EXPORT letstalk-targets DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/letstalk)
