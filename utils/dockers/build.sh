#!/bin/bash -ex
docker build  -f Dockerfile.ubuntu-release --tag registry.gitlab.com/neurochaintech/core/ubuntu/release:18.10  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu/release:18.10
docker build  -f Dockerfile.ubuntu --tag registry.gitlab.com/neurochaintech/core/ubuntu:18.10  . && docker push registry.gitlab.com/neurochaintech/core/ubuntu:18.10
docker build  -f Dockerfile.fedora --tag registry.gitlab.com/neurochaintech/core/fedora:28  . && docker push registry.gitlab.com/neurochaintech/core/fedora:28
docker build  -f Dockerfile.debian --tag registry.gitlab.com/neurochaintech/core/debian:sid  . && docker push registry.gitlab.com/neurochaintech/core/debian:sid
