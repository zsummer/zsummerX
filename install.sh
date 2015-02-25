#!/bin/bash
cp -fr ./depends/log4z/ ./include/log4z
cp -fr ./depends/proto4z/ ./include/proto4z
cp -fr ./depends/rc4/ ./include/rc4
cp -fr ./depends/log4z/lib/ ./lib
destPath="/Users/zhangyawei/Documents/github/breeze"
cp -fr include/ $destPath"/depends_win/include/"
cp -fr lib/ $destPath"/depends_win/lib/"
cp -fr include/ $destPath"/depends_linux/include/"
cp -fr lib/ $destPath"/depends_linux/lib/"
echo "success"
