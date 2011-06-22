#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ] ; then
	echo
	echo "Usage: ./release.sh LOGICAL MAJOR MINOR commit [target]"
	echo
	echo "Creates the release tarball from git sources, tests the compile"
	echo "on local machine, fedora4, fedora5, fedora6."
	echo
	echo "target is an optional filename containing binary package"
	echo "build instructions.  See barrychroots, barrylocal, and"
	echo "barryremote for more information.  The filename must have"
	echo "the word 'root' in it if you wish it to run as root."
	echo
	echo "You can run this multiple times at the same time, for example"
	echo "to build both remote and chroot scripts at once."
	echo
	echo "Note: You may wish to direct the output to a file, for"
	echo "      later examination."
	echo
	echo "Example:   ./release.sh 0 17 0 master"
	echo "           ./release.sh 0 17 0 master barrychroots"
	echo
	exit 1
fi

set -e

# Create the tarball
if [ -f build/barry-$1.$2.$3.tar.bz2 ] ; then
	echo "Tarball already exists... not creating again."
	sleep 2s
else
	./git-release-tar.sh $1 $2 $3 $4
fi

if [ -n "$5" ] ; then
	if echo "$5" | grep root > /dev/null ; then
		# needs root
		su - -c "export BARRYTARBALL=build/barry-$1.$2.$3.tar.bz2 && \
			export BARRYTARBASE=barry-$1.$2.$3.tar.bz2 && \
			export THEMODE=release && \
			export CHOWNUSER=$(whoami) && \
			cd $(pwd) && \
			source $5"
	else
		export BARRYTARBALL=build/barry-$1.$2.$3.tar.bz2
		export BARRYTARBASE=barry-$1.$2.$3.tar.bz2
		export THEMODE=release
		export CHOWNUSER="$(whoami)"

		source $5
	fi
fi

echo
echo "Current build directory:"
ls build
echo
echo "release.sh done"

