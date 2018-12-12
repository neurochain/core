# Architecture 

[TOC]

# Communication 

## Message 

### Format 

![global](../../doc/img/message_format.png "Message format")

Message can be read in two operation: 
* Fixed part containing header size and payload size (each encoded on 32 bits little endian integer).
* Header and payload which are protobuf (v2) messages.

Message description can be found in message.proto 

Every message is signed with the "communication keys (cf [keys](Keys)).

### Pipeline 

![global](../../doc/img/message_pipeline.png "Message pipeline")

## Directory 

A bot comes with a configuration containing trusted bot address: 
* Hostname.
* Port 
* Public key.

The trusted bots act as:
* Entry point to the network.
* Directory to find other bots. 
* Send the tip of the main branch. 

Everytime two bots connect to each others, they exchange credentials and keep them for further use (so every bot act as directory).


## Handshake 

After a tcp connection is established:
The bot that initiate the connection send a Hello message with its port and public key. 
The remote answer with its a peer list (see [Directory](Directory)) and if it want to keep the connection or not. Keeping the connection is based on the actual number of connection a bot already has, to avoid getting a [star network](https://en.wikipedia.org/wiki/Star_network). With those really few rules, we are able to achieve a connected, scalable and decentralized mesh network. 


# Crypto 

## Signature 

[ECDSA](https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm) (ECP256K1 flavor). 

## Keys 

Two types of keys are used: 
* Communication, each bot auto generate a pair the first time it start, and store them on disk (unprotected, and we should do something about it). 
* Transaction (some call them Wallet) keys. Those that "hold" your NCC.

# Rest 

The bot expose a local REST endpoint. API 

# Wallet 

