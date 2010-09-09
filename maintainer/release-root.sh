#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Do not call this script directly.  Call release.sh instead."
	echo
	exit 1
fi

set -e

#
# Build the DEB's
#
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu1004 ubuntu1004
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu910 ubuntu910
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu904 ubuntu904
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu804 ubuntu804
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu710 ubuntu710


#
# Build the Fedora RPM's
#
THESPEC=build/barry-$1.$2/rpm/barry.spec
./make-user-rpm.sh build/barry-$1.$2.tar.bz2 $THESPEC fedora13 f13
./make-user-rpm.sh build/barry-$1.$2.tar.bz2 $THESPEC fedora12 f12
./make-user-rpm.sh build/barry-$1.$2.tar.bz2 $THESPEC fedora11 f11
./make-rpm.sh build/barry-$1.$2.tar.bz2 $THESPEC fedora9 fc9
./make-rpm.sh build/barry-$1.$2.tar.bz2 $THESPEC fedora8 fc8
./make-rpm.sh build/barry-$1.$2.tar.bz2 $THESPEC fedora7 fc7

#
# Build the OpenSUSE RPM's
#
#./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec opensuse10.2 suse10

