#!/bin/sh

#
# Generates the build system.
#

if [ "$1" = "cleanall" ] ; then
	(cd gui && make distclean)
	(cd gui && ./buildgen.sh clean)
	make distclean
	./buildgen.sh clean
elif [ "$1" = "clean" ] ; then
	rm -rf autom4te.cache
	rm -f Makefile.in aclocal.m4 config.guess config.h.in config.sub \
		configure depcomp install-sh ltmain.sh missing \
		src/Makefile.in tools/Makefile.in examples/Makefile.in \
		man/Makefile.in
	# clean up Debian build trails
	rm -rf debian/barry
	rm -f build-arch-stamp build-indep-stamp configure-stamp \
		debian/barry.substvars debian/files \
		tools/bcharge
else
	autoreconf -if
	#autoreconf -ifv
fi

