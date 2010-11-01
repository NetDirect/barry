#!/bin/sh

fullpath() {
	cd "$(dirname "$1")"
	echo "$(pwd)/$(basename "$1")"
}

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo "Usage: abi-prepare.sh tarball abi.xml configure.sh"
	exit 1
fi

TARBALL="$1"
XML="$2"
CONFIG="$3"
VERSION="$(grep -A 1 '<version>' "$XML" | grep -v version | sed "s/^[ 	]//")"

FULLXML="$(fullpath "$XML")"
FULLCONFIG="$(fullpath "$CONFIG")"

# enable errors
set -e

# create target playspace
echo "Creating source tree..."
mkdir "/tmp/$VERSION"			# make sure it doesn't already exist
mkdir -p "/tmp/$VERSION/rootdir"
tar -C "/tmp/$VERSION" -xjf "$TARBALL"

# build and install
echo "Installing to: /tmp/$VERSION/rootdir..."
cd "/tmp/$VERSION/barry-$VERSION"
"$FULLCONFIG" --prefix="/tmp/$VERSION/rootdir"
make -j2
make install

