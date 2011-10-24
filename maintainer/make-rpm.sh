#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" -o -z "$5" ] ; then
	echo
	echo "Usage: ./make-rpm.sh tarball spec_target rpm_args build_dir results_dir"
	echo
	echo "<build_dir>/rpmbuild is the RPM tree where the build will"
	echo "take place.  If rpmbuild does not exist, but SPECS does, then"
	echo "it is assumed build_dir == the rpmbuild dir.  This is useful"
	echo "in the (hopefully) rare occasions where you need to build in"
	echo "/usr/src."
	echo
	echo "tarball is the full pathname of the tarball to extract."
	echo "It is allowed to be relative to the current directory."
	echo "It will be copied to rpmbuild/SOURCES, and the entire tar tree"
	echo "will be extracted into <builddir>/binarybuild,"
	echo "in order to fetch the spec file."
	echo
	echo "spec_target is the filename of the tarball spec file to use"
	echo "when building.  For example, in most cases it will be"
	echo "rpm/barry.spec.  This file will be copied into rpmbuild/SPECS"
	echo "as 'barry.spec' and will be used to build the binary packages."
	echo "If the string is empty, this step will be skipped."
	echo
	echo "rpm_args is the full command line to build the package."
	echo
	echo "<results_dir>/results/ is where the resulting RPM and SRC RPM"
	echo "packages will be copied."
	echo
	exit 1
fi

TARBALL="$1"
SPECPATH="$2"
RPMTARGETS="$3"
RPMPATH="$4"
if [ -d "$RPMPATH/rpmbuild/SPECS" ] ; then
	RPMPATH="$4/rpmbuild"
fi
BUILDPATH="$RPMPATH/binarybuild"
DESTPATH="$5/results"

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
rm -f "$RPMPATH"/RPMS/*/* "$RPMPATH"/SRPMS/*

# expand source
tar -C "$BUILDPATH" -xjvf "$TARBALL"

# setup RPM build tree
cp "$TARBALL" "$RPMPATH/SOURCES"
if [ -n "$SPECPATH" ] ; then
	cp "$BUILDPATH"/barry-*/"$SPECPATH" "$RPMPATH/SPECS/barry.spec"
fi

# build binary packages
(cd "$RPMPATH"/SPECS && $RPMTARGETS)

# move results to destination directory
mv $(find "$RPMPATH/RPMS" "$RPMPATH/SRPMS" -type f -name "*barry*.rpm" -print) "$DESTPATH"

