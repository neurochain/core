#!/bin/bash

mkdir /tmp/db && nohup mongod --dbpath /tmp/db &
sleep 5
pwd;ls -l
./main -c conf/bot.json
