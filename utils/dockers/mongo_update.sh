#!/bin/bash -ex
pushd $(pwd)
dir=$(mktemp -d)
cd $dir

wget https://api.testnet.neurochaintech.io/testnet.tar
tar xf testnet.tar.gz
mongorestore --drop  --uri mongodb://mongo:27017 testnet 
#rm -r $dir
popd 
