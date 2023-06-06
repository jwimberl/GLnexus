# Dockerfile for building GLnexus. The resulting container image runs the unit tests
# by default. It has in its working directory the statically linked glnexus_cli
# executable which can be copied out.
FROM glnexus_builder as glnexus_cli
ENV LC_ALL C.UTF-8
ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive
ARG build_type=Release

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
RUN apt-get -qq update && apt-get -qq install -y libjemalloc2 bcftools tabix pv libcurl4

ENV LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2
COPY --from=glnexus_cli /GLnexus/glnexus_cli /usr/local/bin/
COPY --from=glnexus_cli /GLnexus/external/src/htslib /usr/local/htslib
ADD https://github.com/mlin/spVCF/releases/download/v1.0.0/spvcf /usr/local/bin/
RUN chmod +x /usr/local/bin/spvcf

# specify paths needed for htslib plugin loading
ENV HTS_PATH /usr/local/htslib
ENV LD_LIBRARY_PATH /usr/local/htslib:$LD_LIBRARY_PATH

CMD glnexus_cli
