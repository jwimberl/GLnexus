cmake_minimum_required(VERSION 3.10)

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
project(GLnexus)

# Dependency directory
set(EXTERNALDIR /GLnexusBuilder/external)
INCLUDE_DIRECTORIES(BEFORE ${EXTERNALDIR}/include)
LINK_DIRECTORIES(${EXTERNALDIR}/lib)
SET(ENV{PATH} ${EXTERNALDIR}/bin:$ENV{PATH})
SET(ENV{PKG_CONFIG_PATH} ${EXTERNALDIR}/lib/pkgconfig)

# ssl, threads
find_package(OpenSSL REQUIRED)
find_package(Threads)

# HTSLIB
set(HTSLIBDIR ${EXTERNALDIR}/src/htslib)
set(HTSLIB_INCLUDE_DIR ${HTSLIBDIR}/htslib)
include_directories(${HTSLIB_INCLUDE_DIR})
add_library(libhts STATIC IMPORTED)
set(LIBHTS_A ${HTSLIBDIR}/libhts.a)
set_property(TARGET libhts PROPERTY IMPORTED_LOCATION ${LIBHTS_A})

# RocksDB
set(ROCKSDBDIR ${EXTERNALDIR}/src/rocksdb)
set(ROCKSDB_INCLUDE_DIR ${ROCKSDBDIR}/include)
include_directories(${ROCKSDB_INCLUDE_DIR})
set(ROCKSDB_UTIL_DIR ${ROCKSDBDIR}/util)
include_directories(${ROCKSDB_UTIL_DIR})
add_library(librocksdb STATIC IMPORTED)
set(LIBROCKSDB_A ${ROCKSDBDIR}/librocksdb.a)
set_property(TARGET librocksdb PROPERTY IMPORTED_LOCATION ${LIBROCKSDB_A})

# yaml-cpp
set(YAMLCPPDIR ${EXTERNALDIR}/src/yaml-cpp)
set(YAML_CPP_INCLUDE_DIR ${YAMLCPPDIR}/include)
include_directories(${YAML_CPP_INCLUDE_DIR})
add_library(libyaml-cpp STATIC IMPORTED)
set(LIBYAML_CPP_A ${YAMLCPPDIR}/libyaml-cpp.a)
set_property(TARGET libyaml-cpp PROPERTY IMPORTED_LOCATION ${LIBYAML_CPP_A})

# CTPL
set(CTPLDIR ${EXTERNALDIR}/src/CTPL)
set(CTPL_INCLUDE_DIR ${CTPLDIR})
include_directories(${CTPL_INCLUDE_DIR})

# fcmm
set(FCMMDIR ${EXTERNALDIR}/src/fcmm)
set(FCMM_INCLUDE_DIR ${FCMMDIR})
include_directories(${FCMM_INCLUDE_DIR})

# spdlog
set(SPDLOGDIR ${EXTERNALDIR}/src/spdlog)
set(SPDLOG_INCLUDE_DIR ${SPDLOGDIR}/include)
include_directories(${SPDLOG_INCLUDE_DIR})

# capnp
set(CAPNPDIR ${EXTERNALDIR}/src/capnp)

execute_process(COMMAND git describe --tags --long --dirty --always
                OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_REVISION=\"\\\"${GIT_REVISION}\\\"\" -std=c++14 -Wall -Werror=return-type -Werror=unused-result -Wno-sign-compare -Wno-write-strings -Wno-terminate -fdiagnostics-color=auto -march=ivybridge")
set(CMAKE_CXX_FLAGS_RELEASE "-gdwarf -DNDEBUG -O3")

################################
# Normal Libraries & Executables
################################
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)
include_directories(${COMMON_INCLUDES} ${EXTERNALDIR}/include)
include_directories(${COMMON_INCLUDES} src)
include_directories(${COMMON_INCLUDES} src/rocksdb/include)
include_directories(${COMMON_INCLUDES} capnp/serialize)

# Invoke cap'n proto IDL compiler, to generate C++ serialization code from
# definitions.
add_custom_command(
  OUTPUT  capnp/serialize/defs.capnp.c++ capnp/serialize/defs.capnp.h
  COMMAND ${EXTERNALDIR}/bin/capnp compile -I${EXTERNALDIR}/include -o${EXTERNALDIR}/bin/capnpc-c++ capnp/serialize/defs.capnp
  DEPENDS capnp/serialize/defs.capnp
  COMMENT "GLnexus: generating cap'n proto C++ and H files")

add_library(glnexus
            capnp/serialize/defs.capnp.h capnp/serialize/defs.capnp.c++
            include/types.h src/types.cc
            include/data.h src/data.cc
            include/compare_queries.h src/compare_queries.cc
            include/diploid.h src/diploid.cc
            include/service.h src/service.cc
            include/discovery.h src/discovery.cc
            include/unifier.h src/unifier.cc
            include/genotyper.h src/genotyper.cc
            src/genotyper_utils.h
            src/BCFKeyValueData_utils.h
            include/residuals.h src/residuals.cc
            include/KeyValue.h src/KeyValue.cc
            include/BCFSerialize.h src/BCFSerialize.cc
            include/BCFKeyValueData.h src/BCFKeyValueData.cc
            include/BCF_utils.h src/BCF_utils.cc
            include/RocksKeyValue.h src/RocksKeyValue.cc
            include/cli_utils.h src/cli_utils.cc
            test/utils.cc)
add_library(libglnexus ALIAS glnexus)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -march=ivybridge -static-libstdc++")

add_executable(glnexus_cli cli/glnexus_cli.cc)
set_property(TARGET glnexus_cli PROPERTY ENABLE_EXPORTS 1)
add_dependencies(glnexus_cli libglnexus)
target_link_libraries(glnexus_cli PRIVATE glnexus libhts librocksdb libyaml-cpp libz.a libsnappy.a libbz2.a libzstd.a liblzma.a librt.a libcapnp.a libkj.a OpenSSL::SSL ${CMAKE_DL_LIBS} Threads::Threads)

install(TARGETS glnexus_cli DESTINATION bin)

################################
# Testing
################################

set(CATCHDIR ${EXTERNALDIR}/src/catch)
include_directories(${CATCHDIR}/single_include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
add_executable(unit_tests
                src/diploid.cc test/diploid.cc
                test/unit_tests.cc
                test/htslib_behaviors.cc
                test/rocks_behaviors.cc
                test/types.cc
                test/genotyper.cc
                test/service.cc
                test/gvcf_test_cases.cc
                test/BCFKeyValueData.cc
                test/rocks_integration.cc
                test/unifier.cc
                test/cli_utils.cc
                test/cli.cc)
add_dependencies(unit_tests libglnexus)
target_link_libraries(unit_tests PRIVATE glnexus libhts librocksdb libyaml-cpp libz.a libsnappy.a libbz2.a libzstd.a liblzma.a librt.a libcapnp.a libkj.a OpenSSL::SSL ${CMAKE_DL_LIBS} Threads::Threads)

include(CTest)
add_test(NAME unit_tests COMMAND ./unit_tests -d yes)
add_test(NAME jemalloc_warning COMMAND test/jemalloc_linking.sh)
add_test(NAME getting_started COMMAND test/getting_started.sh)
