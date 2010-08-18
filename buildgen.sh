#!/bin/sh

libtoolit() {
	# Different name on the Mac
	libtoolize --force --copy || glibtoolize --force --copy
}

doconf() {
	aclocal -I $1 && \
		autoheader && \
		automake --add-missing --copy --foreign && \
		autoconf
}

cleangettext() {
	rm -f ABOUT-NLS \
		gui/ABOUT-NLS \
		gui/po/Makefile.in.in \
		gui/po/Makevars.template \
		gui/po/fr.gmo \
		gui/po/stamp-po \
		m4/codeset.m4 \
		m4/gettext.m4 \
		m4/glibc2.m4 \
		m4/glibc21.m4 \
		m4/iconv.m4 \
		m4/intdiv0.m4 \
		m4/intl.m4 \
		m4/intldir.m4 \
		m4/intlmacosx.m4 \
		m4/intmax.m4 \
		m4/inttypes-pri.m4 \
		m4/inttypes_h.m4 \
		m4/lcmessage.m4 \
		m4/lib-ld.m4 \
		m4/lib-link.m4 \
		m4/lib-prefix.m4 \
		m4/lock.m4 \
		m4/longlong.m4 \
		m4/nls.m4 \
		m4/po.m4 \
		m4/printf-posix.m4 \
		m4/progtest.m4 \
		m4/size_max.m4 \
		m4/stdint_h.m4 \
		m4/uintmax_t.m4 \
		m4/visibility.m4 \
		m4/wchar_t.m4 \
		m4/wint_t.m4 \
		m4/xsize.m4 \
		po/Makefile.in.in \
		po/Makevars.template \
		po/fr.gmo \
		po/stamp-po
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
	cleangettext
elif [ "$1" = "clean" ] ; then
	rm -rf autom4te.cache
	rm -f Makefile.in aclocal.m4 config.guess config.h.in config.sub \
		configure depcomp install-sh ltmain.sh missing \
		src/Makefile.in tools/Makefile.in examples/Makefile.in \
		man/Makefile.in INSTALL config.h.in~ compile
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

	# Autogenerate the gettext PO support files
	# Do this for ./ and gui/ and then zap the gui/m4 directory
	autopoint
	(cd gui && autopoint)
	rm -rf gui/m4

	# If we let autoreconf do this, it will run libtoolize after
	# creating some or all of the configure files.  For example,
	# it might copy files into ../m4 again while processing the
	# opensync-plugin/ directory, making those files newer than
	# the gui/configure file.  This will cause configure to
	# be regenerated (incorrectly) during the make step on some
	# systems (Fedora 11).
	#
	# So... we do the libtool stuff all at once at the beginning,
	# then the rest.
	libtoolit m4
	(cd gui && libtoolit ../m4)
	(cd opensync-plugin && libtoolit ../m4)
	(cd opensync-plugin-0.4x && libtoolit ../m4)

	# Now for aclocal, autoheader, automake, and autoconf
	doconf m4
	(cd gui && doconf ../m4)
	(cd opensync-plugin && doconf ../m4)
	(cd opensync-plugin-0.4x && doconf ../m4)
fi

