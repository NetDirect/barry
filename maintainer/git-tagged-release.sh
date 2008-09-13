#!/bin/sh

if [ -z "$1" -o -z "$2" ] ; then
	echo
	echo "Usage: ./git-tagged-tar.sh MAJOR MINOR"
	echo
	echo "MAJOR is the desired major version number"
	echo "MINOR is the desired minor version number"
	echo
	echo "This script is an alternative to git-release-tar.sh, and"
	echo "produces the same output, with the additional side effect of"
	echo "having 2 pristine tar deltas available to generate the"
	echo "debian and the tar.bz2 tarballs directly from the repository."
	echo
	echo "This script assumes that a tag called 'barry-MAJOR.MINOR' already"
	echo "exists.  This script will create two more tags:"
	echo
	echo "    barry-MAJOR.MINOR.tar.bz2"
	echo "    barry_MAJOR.MINOR.orig.tar.gz"
	echo
	echo "Both tags will contain trees with the same contents as the"
	echo "tarballs of the same name."
	echo
	echo "In addition, pristine-tar will be run against the above generated"
	echo "tarballs, and will produce deltas in the usual pristine-tar branch."
	echo
	echo "The main difference between git-release-tar.sh and this script"
	echo "is that this script must be run from inside the root of the"
	echo "git repository you wish to use."
	echo
	echo "Example:  cd barry-repo"
	echo "          git tag barry-0.14"
	echo "          maintainer/git-tagged-tar.sh 0 14"
	echo
	exit 1
fi

DIRNAME="barry-$1.$2"
MAJOR="$1"
MINOR="$2"
COMMIT="barry-$1.$2"

# make sure that git has its tar.umask set to 0022
GITUMASK=$(git config tar.umask)
if [ "$GITUMASK" != "0022" ] ; then
	echo "Please set:"
	echo
	echo "    git config tar.umask 0022"
	echo
	exit 1
fi

set -e

# Are we in a Barry root tree?
if ! [ -f AUTHORS -a -f ChangeLog -a -f COPYING -a -f buildgen.sh ] ; then
	echo "Can't find Barry files. Please run from root of Barry tree."
	exit 1
fi

mkdir maintainer/build

# generate tarball trees
git checkout -b git-tagged-tar "$COMMIT"
maintainer/tar-prepare.sh
git add .
git commit -m "Tarball tree for barry-$MAJOR.$MINOR.tar.bz2"
git tag "barry-$MAJOR.$MINOR.tar.bz2"
git rm -r debian
git commit -m "Tarball tree for barry_$MAJOR.$MINOR.orig.tar.gz"
git tag "barry_$MAJOR.$MINOR.orig.tar.gz"

# generate bzip2 tarball
git archive --prefix="$DIRNAME/" "barry-$MAJOR.$MINOR.tar.bz2" | \
	bzip2 -9c > "maintainer/build/barry-$MAJOR.$MINOR.tar.bz2"

# generate debian source package... create tar.gz without debian/, and
# a tree with debian, and then run dpkg-source to bundle it up
git archive --prefix="$DIRNAME/" "barry_$MAJOR.$MINOR.orig.tar.gz" | \
	gzip -9c > "maintainer/build/barry_$MAJOR.$MINOR.orig.tar.gz"
git archive --prefix="$DIRNAME/" "barry-$MAJOR.$MINOR.tar.bz2" | \
	(cd maintainer/build && tar xf -)
(cd maintainer/build && dpkg-source -b $DIRNAME barry_$MAJOR.$MINOR.orig.tar.gz)

# finally, generate pristine-tar deltas
pristine-tar -m "Release tarball barry-$MAJOR.$MINOR.tar.bz2" commit \
	maintainer/build/barry-$MAJOR.$MINOR.tar.bz2 \
	barry-$MAJOR.$MINOR.tar.bz2
pristine-tar -m "Release tarball barry_$MAJOR.$MINOR.orig.tar.gz" commit \
	maintainer/build/barry_$MAJOR.$MINOR.orig.tar.gz \
	barry_$MAJOR.$MINOR.orig.tar.gz

# cleanup
git checkout master
git branch -D git-tagged-tar

