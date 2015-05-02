#!/bin/bash

rm CMakeCache.txt
rm -fr CMakeFiles
cmake . -DCMAKE_BUILD_TYPE=Release
make -j3
cmake . -DCMAKE_BUILD_TYPE=Debug
make -j3

