#!/bin/sh

#
# Generates the build system.
#

if [ "$1" = "clean" ] ; then
	rm -rf autom4te.cache
	rm -f Makefile.in aclocal.m4 config.guess config.h.in config.sub \
		configure depcomp install-sh ltmain.sh missing \
		images/Makefile.in \
		man/Makefile.in \
		src/Makefile.in src/*.bak \
		src/0.22/Makefile.in \
		src/0.40/Makefile.in \
		INSTALL \
		config.h.in~
else
	autoreconf -if
	#autoreconf -ifv
fi

