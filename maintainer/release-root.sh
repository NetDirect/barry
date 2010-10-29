#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ] ; then
	echo
	echo "Do not call release-root.sh directly.  Call release.sh instead."
	echo
	exit 1
fi

set -e

export BARRYTARBALL=build/barry-$1.$2.$3.tar.bz2
export THESPEC=build/barry-$1.$2.$3/rpm/barry.spec
export THEMODE=release
export CHOWNUSER="$4"

if [ -f ~/.barrychroots ] ; then
	. ~/.barrychroots
else
	. barrychroots
fi

