#!/bin/sh

CHROOT="/var/chroot"
CHOWNUSER="cdfrey:cdfrey"

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ] ; then
	echo
	echo "Usage: ./make-rpm.sh tarball specfile chroot_target short_form"
	echo
	echo "Copies the tarball to the chroot target's redhat SOURCES dir, and"
	echo "the spec file to SPEC dir, then enters the chroot system"
	echo "and runs rpmbuild."
	echo
	echo "short_form is the tag to rename the resulting RPM's with,"
	echo "such as fc5 or fc6."
	echo
	echo "Available chroot targets:"
	find $CHROOT -maxdepth 1 -type d -print | sed "s/^.*\//	/"
	echo
	exit 1
fi

TARPATH="$1"
SPECPATH="$2"
TARNAME=`basename "$TARPATH"`
TARGET="$3"
TAG="$4"

set -e

cp "$TARPATH" "$CHROOT/$TARGET/usr/src/redhat/SOURCES"
cp "$SPECPATH" "$CHROOT/$TARGET/usr/src/redhat/SPECS/barry.spec"
chroot "$CHROOT/$TARGET" /bin/sh -lc "rm -f /usr/src/redhat/RPMS/i386/* /usr/src/redhat/SRPMS/* && cd /usr/src/redhat/SPECS && rpmbuild --target i386 -ba barry.spec --with gui --with opensync && cd /usr/src/redhat/RPMS/i386"
mkdir -p "build/$TARGET"
cp "$CHROOT/$TARGET/usr/src/redhat/RPMS/i386/"* "build/$TARGET"
cp "$CHROOT/$TARGET/usr/src/redhat/SRPMS/"* "build/$TARGET"

# We do this manually in a for loop, since the rename command is
# not the same across various linux distros...
(
	cd "build/$TARGET"
	for f in *.src.rpm ; do
		mv "$f" "$(echo $f | sed "s/.src.rpm$/.$TAG.src.rpm/")"
	done
	for f in *.i386.rpm ; do
		mv "$f" "$(echo $f | sed "s/.i386.rpm$/.$TAG.i386.rpm/")"
	done
)

chown -R $CHOWNUSER "build/$TARGET"

