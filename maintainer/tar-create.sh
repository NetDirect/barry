#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./tar-create.sh MAJOR MINOR"
	echo
	echo "MAJOR is the desired major version number"
	echo "MINOR is the desired minor version number"
	echo
	echo "This script expects a barry-\$MAJOR.\$MINOR directory"
	echo "to exist in the directory it is run in."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2"
MAJOR="$1"
MINOR="$2"

set -e

# Create official release tarball
tar -cvf - $DIRNAME | bzip2 -9c > barry-$MAJOR.$MINOR.tar.bz2

