#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./make-deb-local.sh tarball target_name"
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
TARGET="$2"

set -e

mkdir "build/$TARGET"
cp "$TARPATH" "build/$TARGET"
cd "build/$TARGET"
tar xjvf "$TARNAME"
rm -f "$TARNAME"
cd barry*
debian/rules build
fakeroot -- debian/rules binary

