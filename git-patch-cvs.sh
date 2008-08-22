#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo "Apply unpushed changes from git to a CVS tree."
	echo
	echo "Usage:    git-patch-cvs <git-upstream> <git-head> <cvs-working-dir>"
	echo
	echo "Example: assuming changes on master branch, with remote as"
	echo "         origin/master, patching cvs in ../barry3"
	echo
	echo "     git-patch-cvs origin master ../barry3"
	echo
	echo "This script does NOT push changes to git remote."
	echo
	exit 1
fi

git-cherry "$1" "$2" | \
	sed -n 's/^+ //p' | \
	xargs -L 1 git-cvsexportcommit -cvp -w "$3"

