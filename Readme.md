# Main Page

* Website: [neurochain](https://www.neurochaintech.io) 
* Sources: https://gitlab.com/neurochaintech/core
* Telegram:  https://web.telegram.org/#/im?p=@neurochain_official
* Full documentation: https://neurochaintech.gitlab.io/core/index.html
* Dashboard: http://dashboard.neurochaintech.io:3000/dashboard/snapshot/XawbPu2Y4fJ05xS25tJGZ5wF5V2RRKAJ

# Testnet 

* https://blockexplorer.testnet.neurochaintech.io
* https://wallet.testnet.neurochaintech.io/

# What is it?

## Quick Overview

Neurochain is a blockchain project: understand a decentralized system that write a "log" called a blockchain. 
The "log" is:
* Public.
* It is not possible to change history.


## The Bot

The bot is the Neurochain agent. The agents connect to each others in order to form a mesh network that allow: 
* Secure communication.
* Storing the "log" with sufficient resilience. 

Thanks to a smart [consensus](https://github.com/neurochain/WhitePaper), the bot coalition is able to safely write a unique log. Data written in the log comes 
from external applications to the blockchain. The main and first one is the [cryptocurrency](https://en.wikipedia.org/wiki/Cryptocurrency). Maintaining a 
sustainable network requires rewarding the actors. The second reason for the cryptocurrency is to offer a payment method for other 
applications (e.g. filesharing, traceability).


# Docker Install (recommended)

## Requirements

### Hardware

* 4GB RAM
* 2 CPU (x86_64)
* 30GB disk space (if possible xfs)

### Software

* Ubuntu 18.04 lts
* Docker installed: [https://docs.docker.com/install/linux/docker-ce/ubuntu/](https://docs.docker.com/install/linux/docker-ce/ubuntu/)


## Install

After you have a working docker install:

```
curl -o- -L https://gitlab.com/neurochaintech/core/raw/242-testnet-v4/install.sh |bash 
```


# Source Install (at your own risk)

## Requirements

### Hardware

* 4GB RAM
* 2 CPU (x86_64)
* 30GB disk space (if possible xfs)

### Software

* Ubuntu 18.10
* cmake >=3.0
* clang++8
* libmpfrc++-dev >=3.6


```bash

sudo apt install -y git cmake build-essential libssl-dev mongodb-server libmpfrc++-dev ninja-build clang-8

export CC=clang-8
export CXX=clang++-8

# install pistache
export NCC_WORKDIR=$(pwd)
git clone -n https://github.com/oktal/pistache.git
cd pistache
git checkout c5927e1a12b96492198ab85101912d5d84445f67
mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=~/root/lib/cmake/pistache ..
ninja 
ninja install 


# install bot
cd $NCC_WORKDIR
git clone --branch 179-deploy-testnet-v3 https://gitlab.com/neurochaintech/core.git
cd core
mkdir build
cd build
cmake ..
cmake --build .
```

## Run 

```bash
cd build/src
mkdir conf
cp ../../testnet/{data.0.testnet,bot.json} conf/
./main -c bot.json
```
