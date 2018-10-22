# Technical Choices

[TOC]

# Why keeping X technologies? 

We are trying to keep the code as modular as possible to be able the adapt to different technologies. 
The cost of maintaining them does not represent a significatif overhead when to maintaining modular code. It actually force your to keep it this way.


# Code

## Transport layer 

* Pure TCP: implemented. 
* [ZeroMQ](http://zeromq.org/) comming soon. Both can be used simultaneously. 

## Parser/Serialization 
  
* Protobuf 2: implemented. 
* JSON: might be coming.

### Why protobuf?

* It works.
* Allow easy bindings to other languages.
* Space efficient.

### Why protobuf2 and not 3? 

In early developpment required fields are like "assert" which help us. 

JSON, is handled by protobuf and is easier to use for test/troubleshooting purposes. 

### Why not flatebuffer/captn proto/other 0copy? 

* We might change later, for now protobuf is a robust, well known choice.

### Why not keeping protobuf objects internally? 

* Performance issues. 
* Core code should be independant from communication technologies.


## Crypto++ || why not openssl?

* It works (fill our requirement of security, and algorithm implementation).
* Some [nice people](https://www.cryptopp.com/wiki/Related_Links) use it too 
* We still can "easily" change later.

# Ledger

## Backend

Needs: 
* Portability. 
* Multiple index.
* "Fast"

Candidates where:
* mongodb
* sqlite
* postgresql/mysql
* custom

Criterias: 

|            | Embeded | Scalable         | Flexibility | Devops |
|------------|---------|------------------|-------------|--------|
| mongodb    | no      | yes              | +++         | ++     |
| sqlite     | yes     | no               | ++          | +++    |
| postgresql | no      | it's complicated | +           | +      |
| mysql      | no      | no               | +           | +      |
| custom     | yes     | no               | +++         | +++    |


Flexibility: eeasy to change data type/schema
Devops: effort needed to deploy/have a simple installation.


And the winner is: mongodb. 
The code is modular, so it can be changed later.


# Tooling 

## [Hunter](https://docs.hunter.sh/en/latest/)

It is a pain to manage different targets/distributions, especially for dependencies. A global package manager helps a lot in particular hunter 
that download sources and compile them localy (same envrionnement (compiler, kernel, ...)).

We chose hunter over conan because of available packages lister in the main repo (thanks for Ruslan Baratov for his great work).

## Docker

We are using dockers for daily compilation/run tests. Makes everyone life easier. 
