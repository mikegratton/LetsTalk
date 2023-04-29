# Let's Talk Examples

## pubsub

A basic publish/subscribe pair of applications. This is a good starting point
to understanding the library's API as well as the cmake needed to build applications.

* publisher -- waits for subscriptions. Once one appears, it publishes messages at 
    regular intervals

* subscriber -- echos the messages recieved


## fastdds

This is the baseline eprosima publish/subscribe example.


## reqrep

This shows a basic request/response pattern, including submitting/handling exceptions when
the request is invalid.

## reactor

TODO