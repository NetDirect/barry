DISTCHECK_CONFIGURE_FLAGS = --enable-gui --enable-opensync-plugin --enable-opensync-plugin-4x

ACLOCAL_FLAGS = -I m4
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = COPYING ChangeLog README Doxyfile \
	buildgen.sh \
	data \
	doc \
	hotplug \
	maintainer \
	modprobe \
	ppp \
	rpm \
	test \
	udev

SUBDIRS = po . src tools examples man
# conditionally configured nested subdirectories are listed in $(subdirs)
SUBDIRS += $(subdirs)

pkgconfig_DATA = libbarry-0.pc libbarrydp-0.pc libbarryjdwp-0.pc
if WITH_SYNC
pkgconfig_DATA += libbarrysync-0.pc
endif
if WITH_BACKUP
pkgconfig_DATA += libbarrybackup-0.pc
endif
if WITH_ALX
pkgconfig_DATA += libbarryalx-0.pc
endif

all-local:
	rm -f $(top_builddir)/barry
	ln -s $(top_srcdir)/src $(top_builddir)/barry

clean-local:
	rm -f $(top_builddir)/barry

# these directories will be tested for during make dist to warn if they are
# missing - a new nested package should be added both here and in configure.ac
CONDITIONAL_SUBDIRS = gui opensync-plugin opensync-plugin-0.4x
dist-hook:
	rm -rf `find $(distdir) -name CVS`
	@for subdir in $(CONDITIONAL_SUBDIRS); do \
		if test ! -d $(distdir)/$$subdir; then \
			if test -z "$$MISSING_DIRS"; then \
				echo "###############################################################################"; \
				MISSING_DIRS="--enable-$$subdir"; \
			else \
				MISSING_DIRS="$$MISSING_DIRS --enable-$$subdir"; \
			 fi; \
			echo "ERROR: The $$subdir subdirectory has been omitted!"; \
		else :; fi; \
	done; \
	if test -n "$$MISSING_DIRS"; then \
		echo ; \
		echo "rerun configure with $$MISSING_DIRS before running make dist"; \
		echo "###############################################################################"; \
		exit 1; \
	else :; fi;

