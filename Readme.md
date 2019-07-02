# Main Page

* Website: [neurochain](https://www.neurochaintech.io) 
* Sources: https://gitlab.com/neurochaintech/core
* Telegram:  https://web.telegram.org/#/im?p=@neurochain_official
* Full documentation: https://neurochaintech.gitlab.io/core/index.html
* Dashboard: http://dashboard.neurochaintech.io:3000/dashboard/snapshot/XawbPu2Y4fJ05xS25tJGZ5wF5V2RRKAJ

[TOC]

# What is it?

## Quick Overview

Neurochain is a blockchain project: understand a decentralized system that write a "log" called a blockchain. 
The "log" is:
* Public.
* It is not possible to change history.
* Yes, there is no cryptocurrency here yet.


## The Bot

The bot is the Neurochain agent. The agents connect to each others in order to form a mesh network that allow: 
* Secure communication.
* Storing the "log" with sufficient resilience. 

Thanks to a smart [consensus](https://github.com/neurochain/WhitePaper), the bot coalition is able to safely write a unique log. Data written in the log comes 
from external applications to the blockchain. The main and first one is the [cryptocurrency](https://en.wikipedia.org/wiki/Cryptocurrency). Maintaining a 
sustainable network requires rewarding the actors. The second reason for the cryptocurrency is to offer a payment method for other 
applications (e.g. filesharing, traceability).


# Install and Run 

# Docker (recommended)

## Requirements

* Install docker: https://docs.docker.com/install/ (x86 processor)
* Run the docker

### Basic 

```
docker run -dit --name core --restart always registry.gitlab.com/neurochaintech/core/prod/release:latest
```

This will create and run a docker, running the bot and mongo db. Logs and ledger will be inside the docker, not giving the best performance.

### Advanced

```
docker network create neuro
mkdir mongo # if possible make it an xfs filesystem
docker run -dit --name mongo --network neuro --restart always --log-opt max-size=10m --log-opt max-file=5 -v ./mongo:/data/db  mongo:3.6.8
docker run -dit --name core  --network neuro --restart always --log-opt max-size=10m --log-opt max-file=5 registry.gitlab.com/neurochaintech/core/prod/release:latest ./main -c bot.json
```


# Source 

## Requirements: 
* cmake >=3.0
* g++7/clang++6
* libmpfrc++-dev >=3.6
* pistache >= 

Nothing else is needed (\o/). The rest of the dependencies will be downloaded and compiled by [hunter](http://www.hunter.sh/). 
It makes it easier to cover different platform by having the same version of the dependencies.

### Ubuntu/Debian 

```bash

sudo apt install -y git cmake build-essential libssl-dev mongodb-server libmpfrc++-dev

git clone --branch feature/ledger https://gitlab.com/neurochaintech/core.git
cd core
mkdir build
cd build
cmake ..
cmake --build .
```

## Run 

```bash
cd build/src
./main -c bot.json
```
