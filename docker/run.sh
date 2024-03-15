#!/usr/bin/env bash

cd /build/halm-examples
git pull
git submodule update
rm -rf build build_nor
mkdir build build_nor

# Standard build to run from embedded Flash
cd /build/halm-examples/build
cmake .. ${@}
make -j `grep -c ^processor /proc/cpuinfo`

# Build to run from external NOR Flash
cd /build/halm-examples/build_nor
cmake .. -DTARGET_NOR=ON ${@}
make -j `grep -c ^processor /proc/cpuinfo`
