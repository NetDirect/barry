#!/bin/bash

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./release-bm.sh binary-meta-tarball [target]"
	echo
	echo "Uses the given binary-meta tarball, and builds it on the"
	echo "the target systems using the [target] script."
	echo
	echo "target is an optional filename containing binary package"
	echo "build instructions.  See bmchroots, bmlocal, and"
	echo "bmremote for more information.  The filename must have"
	echo "the word 'root' in it if you wish it to run as root."
	echo
	echo "You can run this multiple times at the same time, for example"
	echo "to build both remote and chroot scripts at once."
	echo
	echo "Note: You may wish to direct the output to a file, for"
	echo "      later examination."
	echo
	echo "Example:   ./release-bm.sh binary-meta.tar.bz2 bmlocal"
	echo
	exit 1
fi

set -e

if [ -n "$2" ] ; then
	if echo "$2" | grep root > /dev/null ; then
		# needs root
		su - -c "export BMTARBALL=$1 && \
			export BMTARBASE=$(basename $1) && \
			export THEMODE=release && \
			export CHOWNUSER=$(whoami) && \
			cd $(pwd) && \
			source $2"
	else
		export BMTARBALL=$1
		export BMTARBASE=$(basename $1)
		export THEMODE=release
		export CHOWNUSER="$(whoami)"

		source $2
	fi
fi

echo
echo "Current build directory:"
ls build
echo
echo "release-bm.sh done"

