add_executable(HelloWorldPublisher HelloWorldPublisher.cpp HelloWorld.cxx HelloWorldPubSubTypes.cxx)
add_executable(HelloWorldSubscriber HelloWorldSubscriber.cpp HelloWorld.cxx HelloWorldPubSubTypes.cxx)

# Note: Link to letstalk to solve all of the include path issues
target_link_libraries(HelloWorldPublisher LetsTalk)
target_link_libraries(HelloWorldSubscriber LetsTalk)
