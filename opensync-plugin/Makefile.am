## Process this file with automake to produce Makefile.in

SUBDIRS = src

EXTRA_DIST = \
	autogen.sh


## The distclean files are just there so we can ship the example correctly. Remove these ##
DISTCLEANFILES = \
	aclocal.m4 \
	config.status \
	Makefile.in \
	stamp-h1 \
	config.h \
	configure \
	config.h.in \
	Makefile \
	config.log \
	src/Makefile \
	src/Makefile.in

distclean-local:
	rm -rf autom4te.cache