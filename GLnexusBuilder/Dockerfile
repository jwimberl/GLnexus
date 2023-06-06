# Dockerfile for GLnexus build dependencies.
FROM ubuntu:18.04 AS builder
ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive
ARG build_type=Release

# apt-get dependencies
RUN apt-get -qq update && \
     apt-get -qq install -y --no-install-recommends --no-install-suggests \
     curl wget ca-certificates git-core less netbase \
     g++ cmake autoconf make file valgrind \
     libjemalloc-dev libzip-dev libsnappy-dev libbz2-dev zlib1g-dev liblzma-dev libzstd-dev \
     python3-pyvcf bcftools pv libcurl4-openssl-dev libssl-dev

# compile other dependencies from source
ADD . /GLnexusBuilder
WORKDIR /GLnexusBuilder
RUN cmake -DCMAKE_BUILD_TYPE=$build_type . && make -j4