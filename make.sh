#!/bin/bash
if [ ! -d lib ]; then
    mkdir lib
fi
cd depends/log4z
bash make.sh $*
cp lib/*.a ../../lib/
cd ../../
if [ ! -d build_linux ]; then
    mkdir build_linux
fi
cd build_linux
if [ $# -gt 0 ] && [ $1 = "max" ]; then
    echo "has search max version" 
    if [ -f /usr/bin/g++-8 ]; then 
        cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-8 -DCMAKE_CXX_COMPILER=/usr/bin/g++-8 $* ../ -DCMAKE_BUILD_TYPE=Debug
        make -j2 
        cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-8 -DCMAKE_CXX_COMPILER=/usr/bin/g++-8 $* ../ -DCMAKE_BUILD_TYPE=Release
        make -j2 

    elif [ -f /usr/bin/g++-6 ]; then 
        cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-6 -DCMAKE_CXX_COMPILER=/usr/bin/g++-6 $* ../ -DCMAKE_BUILD_TYPE=Debug
        make -j2 
        cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-6 -DCMAKE_CXX_COMPILER=/usr/bin/g++-6 $* ../ -DCMAKE_BUILD_TYPE=Release
        make -j2 

    else

    fi
else 
        cmake $* ../ -DCMAKE_BUILD_TYPE=Debug
    make -j2  
    cmake $* ../ -DCMAKE_BUILD_TYPE=Release
    make -j2 


fi

