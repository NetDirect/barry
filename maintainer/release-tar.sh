#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" -o -z "$5" ] ; then
	echo
	echo "Usage: ./release-tar.sh builddir LOGICAL MAJOR MINOR commit"
	echo
	echo "Creates the release tarball from git sources, using the directory"
	echo "builddir."
	echo
	echo "Note: You may wish to direct the output to a file, for"
	echo "      later examination."
	echo
	echo "Example:   ./release-tar.sh barrybuild 0 17 0 master"
	echo
	exit 1
fi

set -e

BUILDDIR="$1"
LOGICAL="$2"
MAJOR="$3"
MINOR="$4"
COMMIT="$5"

mkdir -p "$BUILDDIR"

# Create the tarball
if [ -f $BUILDDIR/barry-$LOGICAL.$MAJOR.$MINOR.tar.bz2 ] ; then
	echo "Tarball already exists... not creating again."
	sleep 2s
else
	./git-release-tar.sh "$BUILDDIR" $LOGICAL $MAJOR $MINOR $COMMIT
fi

echo
echo "release-tar.sh done"

