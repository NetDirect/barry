#!/bin/sh

# Meant to be run from within a git repository.

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo "Usage:  run from within a git repository, as:"
	echo
	echo "     snapshot.sh MAJOR MINOR commit"
	echo
	echo "Results will be located in maintainer/build"
	exit 1
fi

set -e

# Do a little dance to find the full path to snapshot.sh
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"

cd maintainer
./git-extract.sh $1 $2 $3
(cd build/barry* && "$SCRIPTDIR/debian-snap.sh" $1 $2)
(cd build/barry* && "$SCRIPTDIR/rpm-snap.sh" $1 $2)
sleep 2s
(cd build/barry* && ../../tar-prepare.sh)
(cd build && ../tar-create.sh $1 $2)
(cd build && ../deb-src-create.sh $1 $2)

echo
echo "***********************************************************"
echo " Remember to update the OBS RPM spec file with a new"
echo " Release date, as well as updating it with any changes"
echo " in the git tree since the last update."
echo "***********************************************************"
echo

