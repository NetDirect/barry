#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Do not call this script directly.  Call release.sh instead."
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

# Might as well build the RPM's while we're at it
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora5 fc5
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora6 fc6
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora7 fc7
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec opensuse10.2 suse10

