# Architecture 

[TOC]

![global](../../doc/archi.svg "Global Architectur")

# Message 

A [message](@ref neuro::messages::Message) has a [header](@ref neuro::messages::Header) and one of several ([body](@ref neuro::messages::Body)).

The [body](@ref neuro::messages::Body) (or submessage) can be: 
* A transaction.
* A handshake. 
* Ping
* ... 


## Workflow 

Its structure follows the standart encapsulation method
1. TransportLayer has its own header:
   1. Payload size
   2. Payload signature
2. Parser/serializer
   1. Extract header: protocol version, message id and send public key.
   2. Extract body messages (e.g. transaction, handshake, ping,...)
3. Transport layer verify the signature with the extracted key.
4. Message is sent to subscribers.

## misc 


Why putting the sender key in the payload and not in the [TransportLayer](neuro::networking::TransportLayer)?
Sender key is needed by the application, and needs to be in the [header](@ref neuro::messages::Header). 

