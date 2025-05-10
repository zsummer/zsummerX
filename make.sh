#!/bin/bash
if [ ! -d lib ]; then
    mkdir lib
fi


if [ ! -d build ]; then
    mkdir build
fi

if [ ! -d build_select ]; then
    mkdir build_select
fi


cd build
cmake $* ../ -DUSE_SELECT_IMPL=OFF -DCMAKE_BUILD_TYPE=Release
make -j4 

cmake $* ../ -DUSE_SELECT_IMPL=OFF -DCMAKE_BUILD_TYPE=Debug
make -j4  

cd build_select
cmake $* ../ -DUSE_SELECT_IMPL=ON -DCMAKE_BUILD_TYPE=Release
make -j4 

cmake $* ../ -DUSE_SELECT_IMPL=ON -DCMAKE_BUILD_TYPE=Debug
make -j4  

