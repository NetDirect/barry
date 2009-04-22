#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./deb-src-create.sh MAJOR MINOR"
	echo
	echo "MAJOR is the desired major version number"
	echo "MINOR is the desired minor version number"
	echo
	echo "This script expects a barry-\$MAJOR.\$MINOR directory"
	echo "to exist in the directory it is run in."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2"
MAJOR="$1"
MINOR="$2"

set -e

# Create Debian source package, by creating a patched and "orig" set of trees.
#
# The following dance is to keep the .orig.tar.gz containing barry-0.12/
# as the directory, and not something like barry-0.12.orig/ which gets
# corrected by Debian tools.  If there is a better way, please send a patch.
cp -a $DIRNAME $DIRNAME.patched
rm -rf $DIRNAME/debian
tar -cf - $DIRNAME | gzip -9c > barry_$MAJOR.$MINOR.orig.tar.gz
rm -rf $DIRNAME
mv $DIRNAME.patched $DIRNAME
dpkg-source -b $DIRNAME barry_$MAJOR.$MINOR.orig.tar.gz

