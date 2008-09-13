#!/bin/sh

if [ -z "$1" ] ; then
	echo "Usage: tarinsert.sh compress_option tarball tag "
	echo
	echo "Example:  tarinsert.sh -z barry-0.0.1.tar.gz barry-0.0.1"
	echo
	exit 1
fi

set -e
set -x

git checkout -b tarinsert $3
git rm -r .
tar $1 -xvf $2
mv $3/* .

set +e
mv $(find $3 -type f -print) .
set -e

rmdir $3
git add .

set +e
git status
git commit -m "Release tarball for $3"
set -e

git tag $(basename $2)
git checkout master
git clean -xdf
git branch -D tarinsert
echo "Done!"

