#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ] ; then
	echo
	echo "Usage: ./git-release-tar.sh LOGICAL MAJOR MINOR commit"
	echo
	echo "LOGICAL is the desired logical version number"
	echo "MAJOR is the desired libmajor version number"
	echo "MINOR is the desired libminor version number"
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

DIRNAME="barry-$1.$2.$3"
TAGNAME="barry-$1_$2_$3"
LOGICAL="$1"
MAJOR="$2"
MINOR="$3"
COMMIT="$4"
RUNDIR="$(cd "$(dirname "$0")" && pwd)"

set -e

"$RUNDIR/git-extract.sh" $LOGICAL $MAJOR $MINOR $COMMIT
(cd build/$DIRNAME && "$RUNDIR/tar-prepare.sh")
(cd build && "$RUNDIR/tar-create.sh" $LOGICAL $MAJOR $MINOR)
(cd build && "$RUNDIR/deb-src-create.sh" $LOGICAL $MAJOR $MINOR)

