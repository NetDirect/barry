#!/bin/bash

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./release.sh builddir target"
	echo
	echo "Builds release binaries using the tar release in builddir."
	echo
	echo "target is a filename containing binary package"
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
	echo "Example:   ./release.sh barrybuild barrychroots"
	echo
	exit 1
fi

set -e

BUILDDIR="$1"
TARGETFILE="$2"

TARBALL=$(echo $BUILDDIR/barry-*.*.*.tar.bz2)
BASENAME=$(basename $TARBALL)

# Make sure tarball exists
if [ -f $TARBALL ] ; then
	echo "Tarball already exists... continuing..."
	sleep 2s
else
	echo "Tarball does not exist, run release-tar.sh first."
	exit 1
fi

# Build the binary packages by running the target script
if echo "$TARGETFILE" | grep root > /dev/null ; then
	# needs root
	su - -c "export BARRYTARBALL=$TARBALL && \
		export BARRYTARBASE=$BASENAME && \
		export BARRYBUILDDIR=$BUILDDIR && \
		export THEMODE=release && \
		export CHOWNUSER=$(whoami) && \
		cd $(pwd) && \
		source $5"
else
	export BARRYTARBALL=$TARBALL
	export BARRYTARBASE=$BASENAME
	export BARRYBUILDDIR=$BUILDDIR
	export THEMODE=release
	export CHOWNUSER="$(whoami)"

	source $5
fi

echo
echo "Current build directory:"
ls "$BUILDDIR"
echo
echo "release.sh done"

