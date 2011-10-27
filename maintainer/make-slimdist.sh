#!/bin/bash

set -e

if [ -z "$1" ] ; then
	echo "Usage: make-slimdist.sh builddir"
	exit 1
fi

BUILDDIR="$1"

#
# Creates a tarball of just the repository metadata, not the binary
# packages themselves.
#

tar --exclude=*.deb --exclude=*.rpm -cvz -C "$BUILDDIR" -f metadata.tar.gz .

