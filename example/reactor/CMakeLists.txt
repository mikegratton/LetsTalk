
IdlTarget(openerIdl PATH idl SOURCE OpenGarage.idl)

add_executable(garageOpener GarageOpener.cpp)
target_link_libraries(garageOpener PUBLIC openerIdl lt::LetsTalk)
add_executable(garageDoor GarageDoor.cpp)
target_link_libraries(garageDoor PUBLIC openerIdl lt::LetsTalk)
