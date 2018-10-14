FROM registry.gitlab.com/neurochaintech/core/ubuntu:18.04

USER root
COPY build/src/main /bin/core
RUN chmod +x /bin/core

USER neuro