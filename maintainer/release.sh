#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ] ; then
	echo
	echo "Usage: ./release.sh LOGICAL MAJOR MINOR commit"
	echo
	echo "Creates the release tarball from git sources, tests the compile"
	echo "on local machine, fedora4, fedora5, fedora6."
	echo
	echo "Note: You may wish to direct the output to a file, for"
	echo "      later examination."
	echo
	echo "Example:   ./release.sh 0 17 0 master"
	echo
	exit 1
fi

set -e

export CHOWNUSER="$(whoami)"

# Create the tarball
./git-release-tar.sh $1 $2 $3 $4

# Build as root first, so all prompts are finished at the start,
# for the chroot systems...
su - -c "cd $(pwd) && ./release-root.sh $1 $2 $3 '$CHOWNUSER'"

# Build local packages
export BARRYTARBALL=build/barry-$1.$2.$3.tar.bz2
export THESPEC=build/barry-$1.$2.$3/rpm/barry.spec
export THEMODE=release

if [ -f ~/.barrylocal ] ; then
	. ~/.barrylocal
else
	. barrylocal
fi

ls build
echo "Binary packages successfully built"

