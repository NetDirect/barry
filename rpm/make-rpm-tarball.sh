#!/bin/sh

#
# Builds a source tarball suitable for the application release RPM.
#

DIRNAME="$(grep "^Source:" barry.spec | sed "s/^Source: *//;s/.tar.*$//")"

echo "Building tarball for: $DIRNAME"
echo "Press ENTER to continue..."
read

mkdir "$DIRNAME"

cp	../tools/bcharge.cc \
	../udev/10-blackberry.rules \
	barry.spec \
	"$DIRNAME"

tar czf "$DIRNAME.tar.gz" "$DIRNAME"

