#!/bin/sh

if [ -z "$1" ] ; then
	echo "Usage: Run from inside the directory you want to patch."
	echo "       Then do:"
	echo
	echo "       sync-cwd.sh ../path/to/dir/with/changes"
	echo
	echo "Changes will be applied to your current working directory."
	exit 1
fi

DIR="$1"
PATCH="../sync-cwd.patch"

(cd "$DIR" && make clean)
diff -ruN --exclude=CVS --exclude=.git . "$DIR" > "$PATCH"
patch -p1 < "$PATCH"
rm "$PATCH"

