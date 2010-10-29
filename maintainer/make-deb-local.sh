#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" -o -z "$5" ] ; then
	echo
	echo "Usage: ./make-deb-local-root.sh tarball LOGICAL MAJOR MINOR target_name"
	echo
	echo "Extracts tarball in temporary directory and builds Debian"
	echo "packages based on the internal debian scripts.  Assumes"
	echo "that it is running on a development Debian system."
	echo
	echo "target_name is the directory under build/ that the .deb"
	echo "files will be placed into."
	echo
	exit 1
fi

TARPATH="$1"
TARNAME=`basename "$TARPATH"`
LOGICAL="$2"
MAJOR="$3"
MINOR="$4"
TARGET="$5"

set -e

# Build package in /usr/src so that the dbg packages refer to the source
# code in a user-friendly place.

# start clean
rm -rf "/usr/src/barry-$LOGICAL.$MAJOR.$MINOR"
rm -f /usr/src/*barry*deb

# extract from tarball
tar -C /usr/src -xjvf "$TARPATH"

# build plugin based on available opensync
# first check for 0.22 -dev, which has 0 in the name
if dpkg -l libopensync0-dev ; then
	PLUGIN_TARGET=os22-binary
elif dpkg -l libopensync1-dev ; then
	PLUGIN_TARGET=os4x-binary
fi

# build base debs
(cd "/usr/src/barry-$LOGICAL.$MAJOR.$MINOR" && fakeroot -- debian/rules binary $PLUGIN_TARGET)
mkdir -p "build/$TARGET"
mv /usr/src/*barry*deb "build/$TARGET"
if [ -n "$PLUGIN_TARGET" ] ; then
	mv /usr/src/barry-$LOGICAL.$MAJOR.$MINOR/*.deb "build/$TARGET"
fi

# end clean
rm -rf "/usr/src/barry-$LOGICAL.$MAJOR.$MINOR"
rm -f /usr/src/*barry*deb

