#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$BARRYTARBALL" ] ; then
	echo
	echo "Do not call deblocal.sh directly.  Call release.sh instead."
	echo
	exit 1
fi

TAG="$1"
DEBTARGETS="$2"

if [ "$THEMODE" = "release" ] ; then
	./make-deb-local.sh "$BARRYTARBALL" "$TAG" "$DEBTARGETS"
elif [ "$THEMODE" = "test" ] ; then
	./test-build-local.sh "$BARRYTARBALL"
else
	echo
	echo "Mode not set.  Call release.sh or test.sh instead."
	echo
	exit 1
fi

