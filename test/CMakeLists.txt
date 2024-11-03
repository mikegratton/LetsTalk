include(IdlTarget)
IdlTarget(testIdl
    JSON
    SOURCE
        message.idl
        other.idl
        HelloWorld.idl
        OpenGarage.idl
        Big.idl
    PATH idl)

file(GLOB source CONFIGURE_DEPENDS "*.cpp")

add_executable(ltTest ${source})

target_link_libraries(ltTest PUBLIC LetsTalk testIdl)

add_test( NAME LetsTalkUnitTest COMMAND $<TARGET_FILE:ltTest> )