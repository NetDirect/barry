#!/bin/bash

#
# Note that to use this script, you need to add "%_gpg_name" to your
# ~/.rpmmacros file.
#

set -e

if [ -z "$1" ] ; then
	echo "Usage: make-yum.sh builddir"
	exit 1
fi

BUILDDIR="$1"

# Sign all RPMs found under $BUILDDIR/
rpmsign --addsign $(find $BUILDDIR -name "*.rpm" -print)

# Create the YUM repo files for each architecture
for arch in i386 source-i386 i686 source-i686 x86_64 source-x86_64 ; do
	for dir in $(find $BUILDDIR -name "$arch" -type d -print) ; do
		createrepo $dir
		cp yum/key $dir/RPM-GPG-KEY-binary-meta
	done
done

