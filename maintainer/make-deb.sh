#!/bin/sh

CHROOT="/var/chroot"
CHOWNUSER="cdfrey:cdfrey"
CHROOTUSER="cdfrey"

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: ./make-rpm.sh tarball chroot_target short_form"
	echo
	echo "Copies the tarball to the chroot target's /home/$CHROOTUSER/binarybuild dir,"
	echo "then enters the chroot system and runs the build."
	echo
	echo "short_form is the tag to rename the resulting DEB's with,"
	echo "such as ubuntu710, etc."
	echo
	echo "Available chroot targets:"
	find $CHROOT -maxdepth 1 -type d -print | sed "s/^.*\//	/"
	echo
	exit 1
fi

TARPATH="$1"
TARNAME=`basename "$TARPATH"`
TARGET="$2"
TAG="$3"

set -e

cp "$TARPATH" "$CHROOT/$TARGET/home/$CHROOTUSER"

chroot "$CHROOT/$TARGET" su - "$CHROOTUSER" -c /bin/sh -lc "rm -rf binarybuild && mkdir binarybuild && cd binarybuild && tar xjvf ../$TARNAME && cd * && fakeroot -- debian/rules binary"

mkdir -p "build/$TARGET"
cp "$CHROOT/$TARGET/home/$CHROOTUSER/binarybuild/"*.deb "build/$TARGET"
rm "$CHROOT/$TARGET/home/$CHROOTUSER/$TARNAME"

# We do this manually in a for loop, since the rename command is
# not the same across various linux distros...
(
	cd "build/$TARGET"
	for f in *_i386.deb ; do
		mv "$f" "$(echo $f | sed "s/_i386.deb$/_${TAG}_i386.deb/")"
	done
)

chown -R $CHOWNUSER "build/$TARGET"

