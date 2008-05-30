#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Do not call this script directly.  Call release.sh instead."
	echo
	exit 1
fi

set -e

# Build the DEB's
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu804 ubuntu804
./make-deb.sh build/barry-$1.$2.tar.bz2 ubuntu710 ubuntu710
# Build the RPM's
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora7 fc7
sed "s/libusb-devel/libusb/g;s/gtkmm24/gtkmm2/g;s/libglademm24/libglademm/g" < ../rpm/barry.spec > barry-opensuse.spec
./make-rpm.sh build/barry-$1.$2.tar.bz2 barry-opensuse.spec opensuse10.2 suse10
./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora6 fc6
#./make-rpm.sh build/barry-$1.$2.tar.bz2 ../rpm/barry.spec fedora5 fc5

