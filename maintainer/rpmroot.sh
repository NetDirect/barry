#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$BARRYTARBALL" -o -z "$THESPEC" ] ; then
	echo
	echo "Do not call rpmroot.sh directly.  Call release.sh instead."
	echo
	exit 1
fi

CHROOTTARGET="$1"
TAG="$2"

if [ "$THEMODE" = "release" ] ; then
	./make-rpm.sh "$BARRYTARBALL" "$THESPEC" "$CHROOTTARGET" "$TAG"
elif [ "$THEMODE" = "test" ] ; then
	./test-build.sh "$BARRYTARBALL" "$CHROOTTARGET"
else
	echo
	echo "Mode not set.  Call release.sh or test.sh instead."
	echo
	exit 1
fi

