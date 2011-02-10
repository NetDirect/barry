#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: (from inside root of git repo)"
	echo "         maintainer/tagged-release.sh LOGICAL MAJOR MINOR"
	echo
	echo "Creates the release tarball from git sources, tests the compile"
	echo "on local machine, and available chroots."
	echo
	echo "See the help on git-tagged-release.sh for more detailed info."
	echo
	echo "Note: You may wish to direct the output to a file, for"
	echo "      later examination."
	echo
	exit 1
fi

set -e

export CHOWNUSER="$(whoami)"

echo "WARNING: make sure you have the appropriate libopensync0-dev"
echo "         installed on the local system, for the local deb build."
read

# Create the tarball
maintainer/git-tagged-release.sh $1 $2 $3

# Checkout the tag we're building with
git checkout barry-$1.$2.$3

cd maintainer

# Build as root first, so all prompts are finished at the start,
# for the chroot systems...
su - -c "cd $(pwd) && ./release-root.sh $1 $2 $3 '$CHOWNUSER'"

# Build Debian packages in /usr/src/barry-version
./make-deb-local.sh build/barry-$1.$2.$3.tar.bz2 $1 $2 $3 debian

