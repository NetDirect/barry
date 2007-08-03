#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./release.sh MAJOR MINOR HEAD"
	echo
	echo "Set the environment var CVSROOT to use a different repository."
	echo "Set CVSREP to a different directory if needed."
	echo
	echo "Creates the release tarball from CVS sources, tests the compile"
	echo "on local machine, fedora4, fedora5, fedora6."
	echo
	exit 1
fi

set -e

# Create the tarball
./make-release-tar.sh $1 $2 $3

# Make sure it compiles cleanly on all handy systems
# Local first...
./test-build-local.sh build/barry-$1.$2.tar.bz2

# Then as root, for the chroot systems...
su - -c "cd $(pwd) && ./release-root.sh $1 $2"

