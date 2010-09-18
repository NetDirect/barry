#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" -o -z "$CHROOTUSER" -o -z "$CHOWNUSER" ] ; then
	echo
	echo "Usage: ./make-rpm.sh tarball specfile chroot_target short_form"
	echo
	echo "Copies the tarball to the chroot target's user rpmbuild SOURCES dir, and"
	echo "the spec file to SPEC dir, then enters the chroot system"
	echo "and runs rpmbuild as the default user."
	echo
	echo "short_form is the tag to rename the resulting RPM's with,"
	echo "such as fc5 or fc6."
	echo
	echo "Expects CHROOTUSER to be set appropriately in the environment."
	echo
	exit 1
fi

TARPATH="$1"
SPECPATH="$2"
TARNAME=`basename "$TARPATH"`
TARGET="$3"
TAG="$4"

set -e

cp "$TARPATH" "$TARGET/home/$CHROOTUSER/rpmbuild/SOURCES"
cp "$SPECPATH" "$TARGET/home/$CHROOTUSER/rpmbuild/SPECS/barry.spec"
USERCMD="rm -f /home/$CHROOTUSER/rpmbuild/RPMS/i386/* /home/$CHROOTUSER/rpmbuild/SRPMS/* && cd /home/$CHROOTUSER/rpmbuild/SPECS && rpmbuild --target i386 -ba barry.spec --with gui --with opensync && cd /home/$CHROOTUSER/rpmbuild/RPMS/i386"
chroot "$TARGET" su -c "$USERCMD" - $CHROOTUSER
mkdir -p "build/$TAG"
cp "$TARGET/home/$CHROOTUSER/rpmbuild/RPMS/i386/"* "build/$TAG"
cp "$TARGET/home/$CHROOTUSER/rpmbuild/SRPMS/"* "build/$TAG"

# We do this manually in a for loop, since the rename command is
# not the same across various linux distros...
(
	cd "build/$TAG"
	for f in *.src.rpm ; do
		mv "$f" "$(echo $f | sed "s/.src.rpm$/.$TAG.src.rpm/")"
	done
	for f in *.i386.rpm ; do
		mv "$f" "$(echo $f | sed "s/.i386.rpm$/.$TAG.i386.rpm/")"
	done
)

chown -R "$CHOWNUSER" "build/$TAG"

