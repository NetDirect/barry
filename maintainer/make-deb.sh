#!/bin/bash

# args 3 and 4 are optional
if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./make-deb.sh tarball deb_targets [build_dir results_dir]"
	echo
	echo "Creates <build_dir>/binarybuild, expands the tarball into it,"
	echo "and builds debian packages.  After the build, all deb"
	echo "files are copied into <results_dir>/results/"
	echo "Therefore <build_dir> and <results_dir> can be the same"
	echo "if needed, without harm."
	echo
	echo "If build_dir or results_dir are empty, the current directory,"
	echo "plus the corresponding binarybuild and results directories,"
	echo "are used."
	echo
	echo "tarball is the full pathname of the tarball to extract."
	echo "It is allowed to be relative to the current directory."
	echo
	echo "deb_targets is a full command line of what you want to build."
	echo "For example: fakeroot -- debian/rules binary"
	echo
	exit 1
fi

TARBALL="$1"
DEBTARGETS="$2"
BUILDPATH="$3/binarybuild"
if [ -z "$3" ] ; then
	BUILDPATH="binarybuild"
fi
DESTPATH="$4/results"
if [ -z "$4" ] ; then
	DESTPATH="results"
fi

set -e

#
# Note that all commands below are done from the current directory.
# Where the directory must change, it is done within brackets so that
# we return to the current directory immediately afterward.
#
# This is so that all paths and directories given on the command line
# may be relative, and everything still works.
#

# setup directories
rm -rf "$BUILDPATH"
mkdir -p "$BUILDPATH"
rm -rf "$DESTPATH"
mkdir -p "$DESTPATH"

# expand source
tar -C "$BUILDPATH" -xjvf "$TARBALL"

# build binary packages
if ! (cd "$BUILDPATH"/* && eval $DEBTARGETS) ; then
	echo "DEB build failed"
	exit 1
fi

# move results to destination directory
mv $(find "$BUILDPATH" -type f -name "*.deb" -print) "$DESTPATH"

