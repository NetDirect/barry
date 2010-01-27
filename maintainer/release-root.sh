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
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu904 ubuntu904
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu804 ubuntu804
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu710 ubuntu710


#
# Build the Fedora RPM's
#
./make-user-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora12 f12
./make-user-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora11 f11
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora9 fc9
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora8 fc8
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora7 fc7
#./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora6 fc6
#./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora5 fc5

#
# Build the OpenSUSE RPM's
#
#./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec opensuse10.2 suse10

