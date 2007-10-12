#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./make-deb-local-root.sh tarball MAJOR MINOR target_name"
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
MAJOR="$2"
MINOR="$3"
TARGET="$4"

set -e

# Build package in /usr/src so that the dbg packages refer to the source
# code in a user-friendly place.
tar -C /usr/src -xjvf "$TARPATH"
(cd "/usr/src/barry-$MAJOR.$MINOR" && fakeroot -- debian/rules binary)
mkdir "build/$TARGET"
mv /usr/src/*barry*deb "build/$TARGET"
rm -rf "/usr/src/barry-$MAJOR.$MINOR"

