#!/bin/bash
cp -vfr ./depends/log4z ./include/
cp -vfr ./depends/proto4z ./include/
cp -vfr ./depends/rc4 ./include/
cp -vfr ./depends/log4z/lib ./
destPath="/Users/zhangyawei/Documents/github/breeze"
cp -vfr ./include "$destPath/depends/"
cp -vfr ./lib "$destPath/depends_mac/"

echo "success"
