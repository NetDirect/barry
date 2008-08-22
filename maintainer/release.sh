#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: ./release.sh MAJOR MINOR commit"
	echo
	echo "Creates the release tarball from git sources, tests the compile"
	echo "on local machine, fedora4, fedora5, fedora6."
	echo
	echo "Note: You may wish to direct the output to a file, for"
	echo "      later examination."
	echo
	exit 1
fi

set -e

# Create the tarball
./git-release-tar.sh $1 $2 $3

# Build as root first, so all prompts are finished at the start,
# for the chroot systems...
su - -c "cd $(pwd) && ./release-root.sh $1 $2"

# Build Debian packages in /usr/src/barry-version
./make-deb-local.sh build/barry-$1.$2.tar.bz2 $1 $2 debian

