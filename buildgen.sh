#!/bin/sh

doconf() {
	aclocal -I $1 && \
		libtoolize --force --copy && \
		autoheader && \
		automake --add-missing --copy --foreign && \
		autoconf
}

#
# Generates the build system.
#

if [ "$1" = "cleanall" ] ; then
	make distclean
	(cd gui && make distclean)
	(cd opensync-plugin && make distclean)
	(cd opensync-plugin-0.4x && make distclean)
	./buildgen.sh clean
	(cd gui && ./buildgen.sh clean)
	(cd opensync-plugin && ./buildgen.sh clean)
	(cd opensync-plugin-0.4x && ./buildgen.sh clean)
elif [ "$1" = "clean" ] ; then
	rm -rf autom4te.cache
	rm -f Makefile.in aclocal.m4 config.guess config.h.in config.sub \
		configure depcomp install-sh ltmain.sh missing \
		src/Makefile.in tools/Makefile.in examples/Makefile.in \
		man/Makefile.in INSTALL config.h.in~
	# clean up Debian build trails
	rm -rf debian/barry
	rm -f build-arch-stamp build-indep-stamp configure-stamp \
		debian/barry.substvars debian/files \
		tools/bcharge
	# clean up ctags trails
	rm -f src/tags tools/tags examples/tags \
		gui/src/tags \
		opensync-plugin/src/tags \
		opensync-plugin-0.4x/src/tags
elif [ "$1" = "ctags" ] ; then
	echo "Building ctags..."
	(cd src && ctags -R)
	(cd tools && ctags -R)
	(cd examples && ctags -R)
	(cd gui/src && ctags -R)
	if [ "$2" = "0.22" ] ; then
		(cd opensync-plugin/src && ctags -R)
	fi
	if [ "$2" = "0.4x" ] ; then
		(cd opensync-plugin-0.4x/src && ctags -R)
	fi
	# and one with everything
	ctags -R -f ~/tags-barry --tag-relative=yes

	if [ "$2" = "0.22" ] ; then
		# add opensync library as well (yes, I know this only works for my
		# setup... sorry) :-)
		#OS_DIR=~/software/opensync/svn
		OS_DIR=~/software/opensync/0.22
		if [ -d $OS_DIR ] ; then
			echo "Detected 0.22 opensync source tree, building ctags on it..."
			(cd $OS_DIR && ctags -R -a -f ~/tags-barry --tag-relative=yes)
		fi
	fi

	if [ "$2" = "0.4x" ] ; then
		OS_DIR=~/software/opensync/git
		if [ -d $OS_DIR ] ; then
			echo "Detected 0.4x opensync source tree, building ctags on it..."
			(cd $OS_DIR && ctags -R -a -f ~/tags-barry --tag-relative=yes)
		fi
	fi
else
	#autoreconf -if --include=config
	#autoreconf -ifv --include=config

	# autoreconf doesn't seem to support custom .m4 files in config/ (???)
	# so... do it ourselves
	doconf m4
	(cd gui && doconf ../m4)
	(cd opensync-plugin && doconf ../m4)
	(cd opensync-plugin-0.4x && doconf ../m4)
fi

