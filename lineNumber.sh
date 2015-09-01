#! /bin/bash

header=`find ./include -name "*.h" -exec cat  {} \; |wc -l`
source=`find ./src -name "*.cpp" -exec cat  {} \; |wc -l`
echo "total lines=$((header+source)); header=$header source=$source"
