#!/bin/bash

echo "svn cleanup ..."
svn cleanup
echo "svn clean unversion file ..."
dirty=`svn status | grep "^?"|awk '{print $2}'|xargs`
if [ "$dirty" != "" ]; then
	rm -fr $dirty
	svn cleanup
fi

if [ "$#" == "0" ]; then
	echo "svn clean success."
	exit
fi

dirty=`svn status | grep "^M"|awk '{print $2}' |xargs`
if [ $1 \> 0 ] && [ "$dirty" != "" ]; then
	echo "svn revert all modify file to last update ..."
	svn revert $dirty
	svn cleanup
fi

dirty=`svn status | awk '{print $2}' |xargs`
if [ $1 \> 1 ] && [ "$dirty" != "" ]; then
	echo "svn revert all conflicted file ..."
	svn resolved $dirty
	rm -fr $dirty
	svn revert $dirty
fi

svn up
svn status
echo "svn clean success."

