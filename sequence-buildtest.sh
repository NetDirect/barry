#!/bin/sh

set -e

if [ -z "$1" ] ; then
	echo "Usage:"
	echo "    sequence-buildtest.sh <gitrange>"
	echo
	echo "Example:"
	echo "    sequence-buildtest.sh master..desktop"
	echo
	exit 1
fi

if [ ! -f ./buildtest.sh ] ; then
	echo "Please run this from inside the barry/test directory."
	exit 1
fi

DONEFILE="/tmp/sequence-buildtest-done.txt"

if [ -f "$DONEFILE" ] ; then
	echo "NOTE: $DONEFILE already exists..."
	echo "Will continue with current status in 5 seconds..."
	sleep 5s
fi

COMMITS=$(git rev-list --reverse "$1")

for commit in $COMMITS ; do
	if ! grep "$commit" "$DONEFILE" ; then
		git checkout "$commit"
		./buildtest.sh
		echo "$commit" >> "$DONEFILE"
	else
		echo "Skipping: $commit..."
	fi
done

