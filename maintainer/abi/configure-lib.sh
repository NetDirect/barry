#!/bin/sh

#
# accepts a parameter for --prefix
#

export CXXFLAGS="-Wall -Werror -O0 -g"

./configure --enable-boost --enable-nls --with-zlib "$1"

