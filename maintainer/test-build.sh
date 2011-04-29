#!/bin/sh

FIXME - CHROOTUSER is no longer an environment variable

if [ -z "$1" -o -z "$2" -o -z "$CHROOTUSER" ] ; then
	echo
	echo "Usage: ./test-build.sh tarball chroot_target"
	echo
	echo "Copies the tarball to the chroot target's tmp dir, then"
	echo "enters the chroot system as the build user and builds everything."
	echo
	echo "Expects CHROOTUSER to be set properly in the environment."
	echo
	exit 1
fi

TARPATH="$1"
TARNAME=`basename "$TARPATH"`
TARGET="$2"

set -e

cp "$TARPATH" "$TARGET/tmp"
chroot "$TARGET" su - "$CHROOTUSER" -c /bin/sh -lc "rm -rf testbuild && mkdir testbuild && cd testbuild && tar xjvf \"/tmp/$TARNAME\" && cd * && ./configure --prefix=/home/$CHROOTUSER/testbuild/rootdir && make install && cd gui && export PKG_CONFIG_PATH=/home/$CHROOTUSER/testbuild/rootdir/lib/pkgconfig:\$PKG_CONFIG_PATH && ./configure --prefix=/home/$CHROOTUSER/testbuild/rootdir && make install && cd ../opensync-plugin && ./configure --prefix=/home/$CHROOTUSER/testbuild/rootdir && make && cd ../../rootdir && find && echo \"Press enter to continue...\" && read && cd ../.. && rm -rf testbuild"

