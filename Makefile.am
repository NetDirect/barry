
EXTRA_DIST = COPYING ChangeLog README

SUBDIRS = . src tools examples man

pkgconfig_DATA = libbarry-0.pc

all-local:
	rm -f barry
	ln -s src barry

clean-local:
	rm -f barry

