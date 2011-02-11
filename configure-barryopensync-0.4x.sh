#!/bin/sh

export CC="ccache gcc"
export CXX="ccache g++"

#export CXXFLAGS="-Wall -Werror -pedantic -O0 -g"
#export CXXFLAGS="-Wall -Werror -O0 -g -pg"
export CXXFLAGS="-Wall -Werror -O0 -g"
#export CXX="g++-3.3"
export PKG_CONFIG_PATH=/home/cdfrey/Contract/netdirect/syncberry/cvs/rootdir/lib/pkgconfig:/home/cdfrey/software/opensync/git/rootdir/lib/pkgconfig

./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/rootdir \
	--disable-rpath

