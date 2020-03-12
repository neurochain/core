#!/bin/bash

mkdir /tmp/db && nohup mongod --dbpath /tmp/db &
sleep 5
pwd;ls -l
LD_PRELOAD=/home/neuro/root/lib/cmake/pistache/lib/libpistache.so ./main -c conf/bot.json
