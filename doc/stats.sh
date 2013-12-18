#! /bin/bash

source=`find ./ -name "*.cpp" -type f`
header=`find ./ -name "*.h" -type f`

totalH=0
totalS=0
filesH=0
filesS=0

for file in $header
do
	line=`wc -l $file | awk '{print $1}'`
	totalH=$((totalH+line))
	filesH=$((filesH+1))
done

for file in $source
do
	line=`wc -l $file | awk '{print $1}'`
	totalS=$((totalS+line))
	filesS=$((filesS+1))
done

echo "total lines=$((totalH+totalS)); header files=$filesH lines=$totalH; source files=$filesS lines=$totalS"
