#!/bin/bash -ex
docker build --no-cache -f Dockerfile.ubuntu-release --tag registry.gitlab.com/neurochaintech/core/ubuntu/release:18.04  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu/release:18.04 && echo "ubuntu_release OK"
docker build --no-cache -f Dockerfile.ubuntu --tag registry.gitlab.com/neurochaintech/core/ubuntu:18.04  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu:18.04 && echo "ubuntu_dev OK"
