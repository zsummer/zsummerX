#!/bin/bash
cp -vfr ./depends/log4z/ ./include/log4z
cp -vfr ./depends/proto4z/ ./include/proto4z
cp -vfr ./depends/rc4/ ./include/rc4
cp -vfr ./depends/log4z/lib/ ./lib
destPath="/Users/zhangyawei/Documents/github/breeze"
cp -vfr ./include/ "$destPath/depends/include"
cp -vfr ./lib/ "$destPath/depends_mac/lib"

echo "success"
