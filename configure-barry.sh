#!/bin/sh

CHOICE="system"

#export CXXFLAGS="-Wall -Werror -pedantic -O0 -g -pg"
export CXXFLAGS="-Wall -Werror -O0 -g"
#export CXX="g++-3.3"
#export CXX="g++-3.4"
#export CXX="g++-4.1"
#export CXX="/home/cdfrey/software/gcc/rootdir-4.3.0/bin/g++"

export CC="ccache gcc"
export CXX="ccache g++"

export BOOSTOPT="--enable-boost"

if [ -n "$1" ] ; then
	CHOICE="$1"
fi

if [ "$CHOICE" = "devel" ] ; then
echo "Configuring for: devel"
./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir \
	--with-libusb=/home/cdfrey/Contract/netdirect/syncberry/cvs/external/rootdir/libusb \
	$BOOSTOPT
fi


if [ "$CHOICE" = "system" ] ; then
echo "Configuring for: system"
./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir \
	$BOOSTOPT
fi


if [ "$CHOICE" = "stable" ] ; then
echo "Configuring for: stable"
./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir \
	--with-libusb=/home/cdfrey/Contract/netdirect/syncberry/cvs/external/rootdir/libusb-stable \
	$BOOSTOPT
fi

