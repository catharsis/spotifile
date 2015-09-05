#!/bin/sh
if [ $# != 1 ]; then
	echo "Usage: ${0} vX.X.X"
	exit 1;
fi
echo $1 > .version
git add .version
git commit -sv
git tag -m $1 $1
