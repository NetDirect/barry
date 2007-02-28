#!/bin/sh

if [ -z "$CVSROOT" ] ; then
	CVSROOT=":ext:ndprojects@barry.cvs.sourceforge.net:/cvsroot/barry"
fi
if [ -z "$CVSREP" ] ; then
	CVSREP="barry"
fi

echo
echo "Pulling from: $CVSROOT"
echo "Using directory: $CVSREP"

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./make-release-tar.sh MAJOR MINOR"
	echo
	echo "Set the environment var CVSROOT to use a different repository."
	echo "Set CVSREP to a different directory if needed."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2"
TAGNAME="barry-$1_$2"

mkdir build
cd build || exit 1

set -e

cvs -z3 -d "$CVSROOT" \
	co -d $DIRNAME -r $TAGNAME "$CVSREP"
(cd $DIRNAME && ./buildgen.sh)
(cd $DIRNAME/gui && ./buildgen.sh)
rm -rf "$DIRNAME/autom4te.cache" "$DIRNAME/gui/autom4te.cache"
tar --exclude=CVS -cvf - $DIRNAME | gzip -9 > barry-$1.$2.tar.gz

