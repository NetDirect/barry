#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Do not call this script directly.  Call test.sh instead."
	echo
	exit 1
fi

set -e

# Make sure it compiles cleanly on all handy systems
#./test-build.sh build/barry-$1.$2.tar.bz2 fedora4
./test-build.sh build/barry-$1.$2.tar.bz2 fedora5
./test-build.sh build/barry-$1.$2.tar.bz2 fedora6
./test-build.sh build/barry-$1.$2.tar.bz2 fedora7
./test-build.sh build/barry-$1.$2.tar.bz2 opensuse10.2

