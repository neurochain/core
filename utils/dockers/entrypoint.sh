#!/bin/bash

mkdir /tmp/db && nohup mongod --dbpath /tmp/db &
sleep 5
./main -c testnet/bot.json
