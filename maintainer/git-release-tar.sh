#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo
	echo "Usage: ./git-release-tar.sh MAJOR MINOR commit"
	echo
	echo "MAJOR is the desired major version number"
	echo "MINOR is the desired minor version number"
	echo "commit is a git commit tag to use for the extraction"
	echo
	echo "A build directory, containing the packaged results,"
	echo "will be created in the directory in which this script is _run_."
	echo
	echo "It will be assumed _this_ script is inside the git repository"
	echo "you wish to use.  i.e. you are running this script from the"
	echo "same git repository you want to package."
	echo
	echo "Example:  mkdir /tmp/play"
	echo "          cd /tmp/play"
	echo "          /path/to/git/repo/barry/maintainer/git-release-tar.sh 0 14 master"
	echo
	echo "This will create /tmp/play/build containing the results, and"
	echo "use /path/to/git/repo as the repository."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2"
TAGNAME="barry-$1_$2"
MAJOR="$1"
MINOR="$2"
COMMIT="$3"
RUNDIR="$(dirname "$0")/.."

set -e

echo "Using git dir: $RUNDIR"
sleep 3s

mkdir build
(cd "$RUNDIR" && git-archive --format=tar --prefix=$DIRNAME/ $COMMIT) |\
	tar -x -C build -f -
echo "Extracted $COMMIT..."

cd build

# Generate web docs
(cd $DIRNAME/doc/www && ./static.sh)

# Generate doxygen docs
(cd $DIRNAME/src && doxygen && rm -f doxygen.log)

# Generate configure scripts
(cd $DIRNAME && ./buildgen.sh)

# Clean up the extraneous autoconf clutter
rm -rf "$DIRNAME/autom4te.cache" "$DIRNAME/gui/autom4te.cache" "$DIRNAME/opensync-plugin/autom4te.cache"

# Create official release tarball
tar -cvf - $DIRNAME | bzip2 -9c > barry-$MAJOR.$MINOR.tar.bz2

# Create Debian source package, by creating a patched and "orig" set of trees.
#
# The following dance is to keep the .orig.tar.gz containing barry-0.12/
# as the directory, and not something like barry-0.12.orig/ which gets
# corrected by Debian tools.  If there is a better way, please send a patch.
cp -a $DIRNAME $DIRNAME.patched
rm -rf $DIRNAME/debian
tar -cf - $DIRNAME | gzip -9c > barry_$MAJOR.$MINOR.orig.tar.gz
rm -rf $DIRNAME
mv $DIRNAME.patched $DIRNAME
dpkg-source -b $DIRNAME barry_$MAJOR.$MINOR.orig.tar.gz

