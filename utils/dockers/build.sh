#!/bin/bash -ex
docker build --no-cache -f Dockerfile.ubuntu-release --tag registry.gitlab.com/neurochaintech/core/ubuntu/release:18.10  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu/release:18.10
docker build --no-cache -f Dockerfile.ubuntu --tag registry.gitlab.com/neurochaintech/core/ubuntu:18.10  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu:18.10
