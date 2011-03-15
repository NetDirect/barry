#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: ./make-deb-local-root.sh tarball tag targets"
	echo
	echo "Extracts tarball in temporary directory and builds Debian"
	echo "packages based on the internal debian scripts.  Assumes"
	echo "that it is running on a development Debian system."
	echo
	echo "tag is the directory under build/ that the .deb"
	echo "files will be placed into."
	echo
	exit 1
fi

TARPATH="$1"
TARNAME=`basename "$TARPATH"`
TAG="$2"
DEBTARGETS="$3"

BUILDDIR="/usr/src/barry-build"

set -e

# Build package in /usr/src so that the dbg packages refer to the source
# code in a user-friendly place.

# start clean
rm -rf "$BUILDDIR"
mkdir "$BUILDDIR"

# extract from tarball
tar -C "$BUILDDIR" -xjvf "$TARPATH"

# build base debs
(cd "$BUILDDIR"/barry* && fakeroot -- debian/rules $DEBTARGETS)
mkdir -p "build/$TAG"
mv "$BUILDDIR"/*barry*deb "build/$TAG"
mv "$BUILDDIR"/barry*/*.deb "build/$TAG" || echo "No plugin packages found"

# end clean
rm -rf "$BUILDDIR"

