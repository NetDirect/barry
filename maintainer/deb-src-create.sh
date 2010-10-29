#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: ./deb-src-create.sh LOGICAL MAJOR MINOR"
	echo
	echo "LOGICAL is the desired logical version number"
	echo "MAJOR is the desired libmajor version number"
	echo "MINOR is the desired libminor version number"
	echo
	echo "This script expects a barry-\$LOGICAL.\$MAJOR.\$MINOR directory"
	echo "to exist in the directory it is run in."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2.$3"
LOGICAL="$1"
MAJOR="$2"
MINOR="$3"

set -e

# Create Debian source package, by creating a patched and "orig" set of trees.
#
# The following dance is to keep the .orig.tar.gz containing barry-0.17.0/
# as the directory, and not something like barry-0.17.0.orig/ which gets
# corrected by Debian tools.  If there is a better way, please send a patch.
cp -a $DIRNAME $DIRNAME.patched
rm -rf $DIRNAME/debian
tar -cf - $DIRNAME | gzip -9c > barry_$LOGICAL.$MAJOR.$MINOR.orig.tar.gz
rm -rf $DIRNAME
mv $DIRNAME.patched $DIRNAME
dpkg-source -b $DIRNAME barry_$LOGICAL.$MAJOR.$MINOR.orig.tar.gz

