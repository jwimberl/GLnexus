cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(CMAKE_CXX_FLAGS_COVERAGE
    "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C++ compiler during coverage builds."
    FORCE )
SET(CMAKE_C_FLAGS_COVERAGE
    "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C compiler during coverage builds."
    FORCE )
SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used for linking binaries during coverage builds."
    FORCE )
SET(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used by the shared libraries linker during coverage builds."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE )
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Make PROJECT_SOURCE_DIR, PROJECT_BINARY_DIR, and PROJECT_NAME available.
project(GLnexusBuilder)

INCLUDE(ExternalProject)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_BINARY_DIR}/external/include)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/external/lib)
SET(ENV{PATH} ${CMAKE_BINARY_DIR}/external/bin:$ENV{PATH})
SET(ENV{PKG_CONFIG_PATH} ${CMAKE_BINARY_DIR}/external/lib/pkgconfig)

# ssl, threads
find_package(OpenSSL REQUIRED)
find_package(Threads)

# After build, double-check external/src/htslib-stamp/htslib-build-out.log
# to make sure CFLAGS were set as expected
ExternalProject_Add(htslib
    URL https://github.com/samtools/htslib/releases/download/1.9/htslib-1.9.tar.bz2
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND ./configure --prefix=${CMAKE_BINARY_DIR}/external --enable-libcurl --enable-plugins --with-plugin-dir=${CMAKE_BINARY_DIR} --enable-s3
    PATCH_COMMAND sed -i "s/^CFLAGS .*$/CFLAGS = -gdwarf -O3 -DNDEBUG -march=ivybridge/" Makefile
    BUILD_IN_SOURCE 1
    BUILD_COMMAND bash -c "make -n && make -j$(nproc)"
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_BUILD ON
  )

# RocksDB
#
# We set DISABLE_JEMALLOC=1 to avoid link-time dependency to jemalloc; we
# DO however wish to use jemalloc, but expect user to activate it at
# runtime with LD_PRELOAD instead.
#
# Warning: it's dangerously easy to mess with the RocksDB external project
# configuration below and silently get a librocksdb.a compiled without
# optimizations, or targeting an instruction set that's too old or too new.
#
# Here are some checks to perform after any adjustment:
# 1. Verify AVX is used (nonzero results):
#      objdump -d external/src/rocksdb/librocksdb.a | grep vzeroupper | wc -l
# 2. Verify Haswell-and-newer instructions are NOT used (zero results):
#      objdump -d external/src/rocksdb/librocksdb.a | egrep "[[:space:]]andn[[:space:]]"
# 3. Verify accelerated CRC32 is used (nonzero results):
#      objdump -d external/src/rocksdb/librocksdb.a | grep crc32q
# 4. Verify compiler optimizations performed:
#      (TODO better heuristics needed)
#      a) check external/src/rocksdb-stamp/rocksdb-build-out.log to see
#         compiler invocations with -O3
#      b) go through perf/FlameGraph profiling flow and check that no
#         rocksdb function has a big chunk like 5% or more
ExternalProject_Add(rocksdb
    URL https://github.com/facebook/rocksdb/archive/v6.29.3.tar.gz
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND bash -c "export PORTABLE=1 && export DISABLE_JEMALLOC=1 && export OPT='-DNDEBUG -O3 -march=ivybridge -msse4.2 -DHAVE_SSE42 -mpclmul -DROCKSDB_NO_DYNAMIC_EXTENSION' && make -n static_lib && make -j$(nproc) static_lib"
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_BUILD ON
  )

# After build, double-check external/src/yaml-cpp-stamp/yaml-cpp-build-out.log
# to make sure CFLAGS were set as expected
ExternalProject_Add(yaml-cpp
    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.3.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND cmake -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-march=ivybridge .
    BUILD_IN_SOURCE 1
    BUILD_COMMAND bash -c "make -j$(nproc)"
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_BUILD ON
  )

ExternalProject_Add(CTPL
    URL https://github.com/vit-vit/CTPL/archive/v.0.0.2.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
  )

ExternalProject_Add(fcmm
    URL https://github.com/giacomodrago/fcmm/archive/v1.0.1.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
  )

ExternalProject_Add(spdlog
    URL https://github.com/gabime/spdlog/archive/v1.8.2.tar.gz
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
  )

ExternalProject_Add(capnp
    URL https://capnproto.org/capnproto-c++-0.7.0.tar.gz
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
    CONFIGURE_COMMAND ./configure --prefix=${CMAKE_BINARY_DIR}/external
    BUILD_IN_SOURCE 1
    BUILD_COMMAND bash -c "make -j$(nproc)"
    INSTALL_COMMAND make install
    LOG_DOWNLOAD ON
  )

ExternalProject_Add(catch
  URL https://github.com/philsquared/Catch/archive/v1.12.2.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
  )
