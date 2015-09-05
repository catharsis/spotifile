#!/bin/sh
if [ $# != 1 ]; then
	echo "Usage: ${0} vX.X.X"
	exit 1;
fi
git tag -m $1 $1
echo $1 > .version
