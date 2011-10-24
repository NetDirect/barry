#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ] ; then
	echo
	echo "Usage: ./git-extract.sh LOGICAL MAJOR MINOR commit"
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
	echo "          /path/to/git/repo/barry/maintainer/git-extract.sh 0 14 master"
	echo
	echo "This will create /tmp/play/barry-*.*.* containing the results, and"
	echo "use /path/to/git/repo as the repository."
	echo
	exit 1
fi

DIRNAME="barry-$1.$2.$3"
LOGICAL="$1"
MAJOR="$2"
MINOR="$3"
COMMIT="$4"
RUNDIR="$(dirname "$0")/.."

set -e

echo "Using git dir: $RUNDIR"
sleep 3s

umask 0022
(cd "$RUNDIR" && git archive --format=tar --prefix=$DIRNAME/ $COMMIT) |\
	tar -x -f -
echo "Extracted $COMMIT..."

