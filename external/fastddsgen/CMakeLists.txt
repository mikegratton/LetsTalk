install(
    PROGRAMS
        fastddsgen 
        fastddsgen.jar 
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/bin
)
install(
    FILES 
        JsonSupportHeader.stg
        JsonSupportSource.stg
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/share/LetsTalk
)
