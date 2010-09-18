#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Do not call test-root.sh directly.  Call test.sh instead."
	echo
	exit 1
fi

set -e

export BARRYTARBALL=build/barry-$1.$2.tar.bz2
export THESPEC=build/barry-$1.$2/rpm/barry.spec
export THEMODE=test

# Make sure it compiles cleanly on all handy systems
if [ -f ~/.barrychroots ] ; then
	. ~/.barrychroots
else
	. barrychroots
fi

