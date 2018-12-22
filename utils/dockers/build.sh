#!/bin/bash -ex
docker build -f Dockerfile.ubuntu-release --tag registry.gitlab.com/neurochaintech/core/ubuntu/release:18.04  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu/release:18.04
docker build -f Dockerfile.ubuntu --tag registry.gitlab.com/neurochaintech/core/ubuntu:18.04  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu:18.04
