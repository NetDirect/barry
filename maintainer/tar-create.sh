#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: ./tar-create.sh LOGICAL MAJOR MINOR"
	echo
	echo "LOGICAL is the desired logical version number"
	echo "MAJOR is the desired libmajor version number"
	echo "MINOR is the desired libminor version number"
	echo
	echo "This script expects a barry-\$LOGICAL.\$MAJOR.\$MINOR directory"
	echo "to exist in the directory it is run in."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2.$3"
LOGICAL="$1"
MAJOR="$2"
MINOR="$3"

set -e

# Create official release tarball
tar -cvf - $DIRNAME | bzip2 -9c > barry-$LOGICAL.$MAJOR.$MINOR.tar.bz2

