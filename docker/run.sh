#!/usr/bin/env bash

PROJECT=halm-examples

cd /build/${PROJECT}
git pull
git submodule update
rm -rf build build-nor deploy
mkdir build build-nor deploy

# Standard build to run from embedded Flash
cd /build/${PROJECT}/build
cmake .. -DUSE_HEX=ON ${@}
make -j `grep -c ^processor /proc/cpuinfo`
find . -maxdepth 2 -iname "*.hex" -print0 | tar -cvz -f ../deploy/${PROJECT}_build.tar.gz --null -T -
find . -iname "*.elf" -exec arm-none-eabi-size {} \; | grep \.elf | sort -k6 | sed 's/^[ ]\+\([0-9]\+\)[ \t0-9a-f]*\.\/\([\.\-\/_0-9a-zA-Z]\+\)$/\2;\1/' > ../deploy/${PROJECT}_build_analysis.csv

# Build to run from external NOR Flash
cd /build/${PROJECT}/build-nor
cmake .. -DUSE_HEX=ON -DUSE_DFU=ON -DTARGET_NOR=ON ${@}
make -j `grep -c ^processor /proc/cpuinfo`
find . -maxdepth 2 -iname "*.hex" -print0 | tar -cvz -f ../deploy/${PROJECT}_build-nor.tar.gz --null -T -
find . -iname "*.elf" -exec arm-none-eabi-size {} \; | grep \.elf | sort -k6 | sed 's/^[ ]\+\([0-9]\+\)[ \t0-9a-f]*\.\/\([\.\-\/_0-9a-zA-Z]\+\)$/\2;\1/' > ../deploy/${PROJECT}_build-nor_analysis.csv
