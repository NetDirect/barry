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
	echo "       or"
	echo "       ./make-release-tar.sh MAJOR MINOR HEAD"
	echo
	echo "MAJOR is the desired major version number"
	echo "MINOR is the desired minor version number"
	echo "HEAD is a literal string to override the default behaviour"
	echo "     of retrieving the CVS tagged major/minor revision, and"
	echo "     instead, just fetching the HEAD version, and packaging"
	echo "     it up as version MAJOR.MINOR."
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

if [ "$3" != "HEAD" ] ; then
	cvs -z3 -d "$CVSROOT" co -d $DIRNAME -r $TAGNAME "$CVSREP"
	echo "Fetched tag: $TAGNAME"
else
	cvs -z3 -d "$CVSROOT" co -d $DIRNAME "$CVSREP"
	echo "Fetched HEAD"
fi
(cd $DIRNAME && ./buildgen.sh)
(cd $DIRNAME/gui && ./buildgen.sh)
(cd $DIRNAME/opensync-plugin && ./buildgen.sh)
rm -rf "$DIRNAME/autom4te.cache" "$DIRNAME/gui/autom4te.cache" "$DIRNAME/opensync-plugin/autom4te.cache"
tar --exclude=CVS -cvf - $DIRNAME | bzip2 -9c > barry-$1.$2.tar.bz2

