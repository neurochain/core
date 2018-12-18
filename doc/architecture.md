# Architecture 

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [Architecture](#architecture)
- [Communication](#communication)
    - [Message](#message)
        - [Request Reply](#request-reply)
        - [Broadcasted](#broadcasted)
        - [Format](#format)
        - [Pipeline](#pipeline)
    - [Directory](#directory)
    - [Message Sequence](#message-sequence)
        - [Tcp Connection](#tcp-connection)
        - [Hello/World](#helloworld)
        - [Update Ledger](#update-ledger)
- [Crypto](#crypto)
    - [Signature](#signature)
    - [Keys](#keys)
- [Rest](#rest)
- [Wallet](#wallet)
- [Ledger](#ledger)
    - [Update](#update)
- [Miners](#miners)
    - [Mining algorithm](#mining-algorithm)
    - [Writting blocks](#writting-blocks)

<!-- markdown-toc end -->

# Communication 

## Message 

### Request Reply 

* Hello/World
* GetPeers/Peers
* GetBlock/Block

### Broadcasted 

* Block 
* Transaction
* Election Result 

### Format 

![global](../../doc/img/message_format.png "Message format")

Message can be read in two operations:
* Fixed part containing header size and payload size (each encoded on 32 bits little endian integer).
* Header and payload which are protobuf (v2) messages.

Message description can be found in message.proto 

Every message is signed with the "communication keys (cf [keys](Keys)).

### Pipeline 

![global](../../doc/img/message_pipeline.png "Message pipeline")

## Directory 

A bot comes with a configuration containing trusted bots. 
* Endpoint.
* Port.
* Public key.

The trusted bots act as:
* Entry point to the network.
* Directory to find other bots. 
* Send the tip of the main branch (to avoid long range attacks).

Everytime two bots connect to each others, they exchange credentials and keep them for further use (so every bot act as directory).


## Message Sequence

![message sequence](img/message_sequence.png "Message Sequence")


### Tcp Connection 

A bot will try to get to `max_connection` number of connections and accept up to twice that number. 
To connect to a remote bot, a bot needs its endpoint (hostname or IP), port and public key. A remote is randomly chosen in the directory.

### Hello/World

The bot establishing the connection is in charge of sending a Hello message. 
It contains the public key (of the bot connecting) and a optional listening port. 

The remote answers with: 
* A list of peers (see [Directory](Directory)).
* If it accepts the connection. Keeping the connection is based on the actual number of connections a bot already has, to avoid getting a [star network](https://en.wikipedia.org/wiki/Star_network). 
* The tip of the main branch.

With those really few rules, we are able to achieve a connected, scalable and decentralized mesh network. 

Example: 
* Bob has a max `max_connection` of 3. He will try to connect to 3 bots. 
* If Alice, William and Oli try to connect to Bob he will accpet. Now Bob is connected to 6 peers.
* If David tries to connect to Bob, Bob will nicely answer with a World with a peer list, and interrompt connection.

### Update Ledger 

After handshake (hello/worl), a bot needs to update its ledger. It uses the tip given by the world message to recursively ask for a parent block. 

# Crypto 

## Signature 

[ECDSA](https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm) (ECP256K1 flavor). 

## Keys 

Two types of keys are used: 
* Communication, each bot auto generates a pair the first time it starts, and stores them on disk (unprotected, and we should do something about it). 
* Transaction (some call them Wallet) keys. Those that "hold" your NCC.

Security os keys are a let to the user. 

# Rest 

The bot exposes a local REST endpoint (http). 

It allows to request the ledger, and send transaction to the network. 

# Wallet 

![wallet](../../doc/img/wallet.png "Wallet")

Known issues: 
* Either keys or password passed in clear to the wallet bakend.

# Ledger 

Contains the blockchain. It stored in a mongo local database (each bot has its own).
Two collections are used, one containing block headers, and other one containing transactions. 
Blocks have a pointer to previous block (it is a chain after all). To handle forks, blocks are tagged with a path allowing to know if a block is an ancestor of an other in O(1).

Ledger is used to:
* Verify if incomming blocks are legit.
* Create transactions, and compute wallet balance.
* Send blocks to other bots when they need to update their own ledger. 

This last point rely on the identity of the serialization (Protobuf <=> Json <=> Bson), since block are reconstructed for db documents. 

## Update 

When connecting for the first time, or reconnecting after an absence, a bot needs to update its ledger. This is a critical moment for non POW chains as fake are easy to forge. 
During the handshake, bots exchange the tip of their main branch. From there, the bot can ask its peers for parent block to update itself.

# Miners 

Miners need to register by writting a "special" transaction into the blockchain. Later, this transaction will be used to separate keys signing blocks from keys earning Pii.

## Mining algorithm

Mining algorithm are not written (yet). Developpers can write their own using the rest API (or by hacking the actual C++ code). 

## Writting blocks

In order to write blocks (and to be rewarded), miners will need to share their keys to the bot. 
 
