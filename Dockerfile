# Dockerfile for building GLnexus. The resulting container image runs the unit tests
# by default. It has in its working directory the statically linked glnexus_cli
# executable which can be copied out.
FROM ubuntu:18.04 AS builder
MAINTAINER DNAnexus
ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive
ARG build_type=Release

# dependencies
RUN apt-get -qq update && \
     apt-get -qq install -y --no-install-recommends --no-install-suggests \
     curl wget ca-certificates git-core less netbase \
     g++ cmake autoconf make file valgrind \
     libjemalloc-dev libzip-dev libsnappy-dev libbz2-dev zlib1g-dev liblzma-dev libzstd-dev \
     python3-pyvcf bcftools pv libcurl4-openssl-dev libssl-dev

# Copy in the local source tree / build context
ADD . /GLnexus
WORKDIR /GLnexus

# compile GLnexus
RUN cmake -DCMAKE_BUILD_TYPE=$build_type . && make -j4

# specify paths needed for htslib plugin loading
ENV HTS_PATH /GLnexus/external/src/htslib/
ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:/GLnexus/external/src/htslib

# set up default container start to run tests
CMD ctest -V

# Second stage: copy glnexus_cli into a slimmer image
# use ubuntu 20.04+ to get multi-threaded bgzip with libdeflate
FROM ubuntu:20.04
ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -qq update && apt-get -qq install -y libjemalloc2 bcftools tabix pv

ENV LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2
COPY --from=build /GLnexus/external/src/htslib/hfile_gcs.so /usr/local/libexec/htslib/
COPY --from=build /GLnexus/external/src/htslib/hfile_libcurl.so /usr/local/libexec/htslib/
COPY --from=build /GLnexus/external/src/htslib/hfile_s3.so /usr/local/libexec/htslib/
COPY --from=builder /GLnexus/glnexus_cli /usr/local/bin/
ADD https://github.com/mlin/spVCF/releases/download/v1.0.0/spvcf /usr/local/bin/
RUN chmod +x /usr/local/bin/spvcf

# specify paths needed for htslib plugin loading
ENV HTS_PATH /GLnexus/external/src/htslib/
ENV LD_LIBRARY_PATH /GLnexus/external/src/htslib:$LD_LIBRARY_PATH

CMD glnexus_cli
