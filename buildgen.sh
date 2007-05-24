#!/bin/sh

#
# Generates the build system.
#

if [ "$1" = "cleanall" ] ; then
	(cd gui && make distclean)
	(cd gui && ./buildgen.sh clean)
	(cd opensync-plugin && make distclean)
	(cd opensync-plugin && ./buildgen.sh clean)
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
	# clean up ctags trails
	rm -f src/tags tools/tags examples/tags \
		gui/src/tags opensync-plugin/src/tags
elif [ "$1" = "ctags" ] ; then
	(cd src && ctags -R)
	(cd tools && ctags -R)
	(cd examples && ctags -R)
	(cd gui/src && ctags -R)
	(cd opensync-plugin/src && ctags -R)
	# and one with everything
	ctags -R -f ~/tags-barry --tag-relative=yes
else
	autoreconf -if
	#autoreconf -ifv
fi

