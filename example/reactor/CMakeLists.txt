
include(IdlTarget)
IdlTarget(openerIdl SOURCE OpenGarage.idl)

add_executable(garageOpener GarageOpener.cpp)
target_link_libraries(garageOpener PUBLIC openerIdl LetsTalk)
add_executable(garageDoor GarageDoor.cpp)
target_link_libraries(garageDoor PUBLIC openerIdl LetsTalk)
