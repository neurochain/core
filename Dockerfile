FROM registry.gitlab.com/neurochaintech/core/ubuntu:18.04

COPY build/src/main /bin/core
RUN chmod +x /bin/core