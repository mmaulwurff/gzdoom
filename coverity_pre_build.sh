#!/bin/bash

echo ${TRAVIS_BUILD_DIR}
mkdir build
cd build
cmake ${CMAKE_OPTIONS}           \
      -DFORCE_INTERNAL_ZLIB=YES  \
      -DFORCE_INTERNAL_JPEG=YES  \
      -DFORCE_INTERNAL_BZIP2=YES \
      -DFORCE_INTERNAL_GME=YES   \
      -DPK3_QUIET_ZIPDIR=YES     \
      ..
