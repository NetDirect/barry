#!/bin/sh

CHROOT="/var/chroot"
BUILDUSER="cdfrey"

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./test-build.sh tarball chroot_target"
	echo
	echo "Copies the tarball to the chroot target's tmp dir, then"
	echo "enters the chroot system as the build user and builds everything."
	echo
	echo "Available chroot targets:"
	find $CHROOT -type d -maxdepth 1 -print | sed "s/^.*\//	/"
	echo
	exit 1
fi

TARPATH="$1"
TARNAME=`basename "$TARPATH"`
TARGET="$2"

set -e

cp "$TARPATH" "$CHROOT/$TARGET/tmp"
chroot "$CHROOT/$TARGET" su - "$BUILDUSER" -c /bin/sh -c "rm -rf testbuild && mkdir testbuild && cd testbuild && tar xjvf \"/tmp/$TARNAME\" && cd * && ./configure --prefix=/home/$BUILDUSER/testbuild/rootdir && make install && cd gui && export PKG_CONFIG_PATH=/home/$BUILDUSER/testbuild/rootdir/lib/pkgconfig && ./configure --prefix=/home/$BUILDUSER/testbuild/rootdir && make install && cd ../../rootdir && find && echo \"Press enter to continue...\" && read && cd ../.. && rm -rf testbuild"

