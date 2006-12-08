#!/bin/sh

#
# Generates the build system.
#

if [ "$1" = "clean" ] ; then
	rm -rf autom4te.cache
	rm -f Makefile.in aclocal.m4 config.guess config.h.in config.sub \
		configure depcomp install-sh ltmain.sh missing \
		src/Makefile.in tools/Makefile.in
else
	autoreconf -if
	#autoreconf -ifv
fi

