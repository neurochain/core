# Architecture 

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [Architecture](#architecture)
- [Communication](#communication)
    - [Message](#message)
        - [Format](#format)
        - [Pipeline](#pipeline)
    - [Directory](#directory)
    - [Handshake](#handshake)
- [Crypto](#crypto)
    - [Signature](#signature)
    - [Keys](#keys)
- [Rest](#rest)
- [Wallet](#wallet)
- [Ledger](#ledger)
- [Miners](#miners)
    - [Mining algorithm](#mining-algorithm)
    - [Writting blocks](#writting-blocks)

<!-- markdown-toc end -->

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

A bot comes with a configuration containing trusted bots. 
* Hostname.
* Port.
* Public key.

The trusted bots act as:
* Entry point to the network.
* Directory to find other bots. 
* Send the tip of the main branch (to avoid long range attacks).

Everytime two bots connect to each others, they exchange credentials and keep them for further use (so every bot act as directory).


## Handshake 

After a tcp connection is established:
The bot that initiates the connection sends a Hello message with its port and public key. 
The remote answers with its a peer list (see [Directory](Directory)) and if it wants to keep the connection or not. Keeping the connection is based on the actual number of connections a bot already has, to avoid getting a [star network](https://en.wikipedia.org/wiki/Star_network). With those really few rules, we are able to achieve a connected, scalable and decentralized mesh network. 


# Crypto 

## Signature 

[ECDSA](https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm) (ECP256K1 flavor). 

## Keys 

Two types of keys are used: 
* Communication, each bot auto generates a pair the first time it starts, and stores them on disk (unprotected, and we should do something about it). 
* Transaction (some call them Wallet) keys. Those that "hold" your NCC.

# Rest 

The bot exposes a local REST endpoint (http). 

It allows to request the ledger, and send transaction to the network. 

# Wallet 

![wallet](../../doc/img/wallet.png "Wallet")

Known issues: 
* Either keys or password passed in clear to the wallet bakend.

# Ledger 

Contains the blockchain. It stored in a mongo local database (each bot has its own).
Two collections are used, one containing block headers, and other one containing transactions. When 

# Miners 

Miners need to register by writting a "special" transaction into the blockchain. Later, this transaction will be used to separate keys signing blocks from keys earning Pii.

## Mining algorithm

Mining algorithm are not written (yet). Developpers can write their own using the rest API (or by hacking the actual C++ code). 

## Writting blocks

In order to write blocks (and to be rewarded), miners will need to register their into the bot. 
For safety reasons, 
 
