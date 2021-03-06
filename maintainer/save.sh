#!/bin/bash

if [ -z "$1" -o -z "$2" -o \( "$(whoami)" = "root" -a -z "$CHOWNUSER" \) ] ; then
	echo
	echo "Usage: ./save.sh results_dir dir_tag file_tag [script args...]"
	echo
	echo "Runs the given script, and then copies all resulting binaries"
	echo "from <results_dir>/results/ into directory dir_tag and"
	echo "renames all the files to include <file_tag> in the name."
	echo
	echo "file_tag can be an empty string to skip the rename."
	echo
	echo "Expects CHOWNUSER to be set appropriately"
	echo "in the environment if run as root."
	echo
	exit 1
fi

RESULTSDIR="$1"
shift
TAG="$1"
shift
SHORTTAG="$1"
shift

set -e

# run the script
echo "Running: $@"
"$@"

# copy the results back to the target directory
mkdir -p "$TAG"
cp "$RESULTSDIR"/* "$TAG"
if [ "$(whoami)" = "root" ] ; then
	chown -R "$CHOWNUSER" "$TAG"
fi

# We do this manually in a for loop, since the rename command is
# not the same across various linux distros...
if [ -n "$SHORTTAG" ] ; then
	(
	cd "$TAG"

	for f in *.deb ; do
		if [ -f "$f" ] ; then
			mv "$f" "$(echo $f | sed "s/_\([^_]*\)\.deb$/_${SHORTTAG}_\1.deb/")"
		fi
	done

	for f in *.rpm ; do
		if [ -f "$f" ] ; then
			mv "$f" "$(echo $f | sed "s/\.\([^.]*\)\.rpm$/.${SHORTTAG}.\1.rpm/")"
		fi
	done
	)
fi

