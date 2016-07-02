#! /bin/bash

header=`find ./include/zsummerX -type f \( -name "*.cpp" -or -name "*.h" \)  -print0 | xargs -0 cat |grep -v -e ^// -e ^$ -e "^[ ]\*" -e "$\*"| wc -l`
source=`find ./src -type f \( -name "*.cpp" -or -name "*.h" \)  -print0 | xargs -0 cat |grep -v -e ^// -e ^$ -e "^[ ]\*" -e "$\*"| wc -l`
echo "total lines=$((header+source)); header=$header source=$source"
