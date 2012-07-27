ACLOCAL_FLAGS = -I m4

SUBDIRS = po . src

EXTRA_DIST = \
	buildgen.sh \
	patches

dist-hook:
	 rm -rf `find $(distdir) -name CVS`

