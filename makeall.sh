#!/bin/bash
#cmake . -DZSUMMERX_USE_SELECT=1 -DCMAKE_BUILD_TYPE=Release
#make -j3
#cmake . -DZSUMMERX_USE_SELECT=1 -DCMAKE_BUILD_TYPE=Debug
#make -j3
cmake . -DCMAKE_BUILD_TYPE=Release
make -j3
cmake . -DCMAKE_BUILD_TYPE=Debug
make -j3

