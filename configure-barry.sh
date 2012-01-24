#!/bin/sh

#export CXXFLAGS="-Wall -Werror -pedantic -O0 -g -pg"
export CXXFLAGS="-Wall -Werror -O0 -g"

export CC="ccache gcc"
export CXX="ccache g++"

export BOOSTOPT="--enable-boost"

./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/rootdir \
	$BOOSTOPT \
	$1

