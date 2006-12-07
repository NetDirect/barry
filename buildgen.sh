#!/bin/sh

#
# Generates the build system.  Specify "orig" on the command line
# to use the non-autoconf build system.
#

if [ "$1" = "orig" ] ; then
	cp Makefile.orig Makefile
	cp src/Makefile.orig src/Makefile
else
	autoreconf -if
fi

