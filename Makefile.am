DISTCHECK_CONFIGURE_FLAGS = --enable-gui --enable-opensync-plugin --enable-opensync-plugin-4x --enable-desktop

ACLOCAL_FLAGS = -I m4
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = COPYING ChangeLog README Doxyfile AUTHORS KnownBugs \
	buildgen.sh \
	data \
	doc \
	hotplug \
	maintainer \
	modprobe \
	ppp \
	rpm \
	udev \
	bash \
	zsh \
	hal \
	logo \
	contrib \
	menu \
	usbmon-6 \
	android \
	wince \
	.gitignore

SUBDIRS = po . src tools examples man test
# conditionally configured nested subdirectories are listed in $(subdirs)
SUBDIRS += $(subdirs)

pkgconfig_DATA = libbarry-19.pc libbarrydp-19.pc libbarryjdwp-19.pc
if WITH_SYNC
pkgconfig_DATA += libbarrysync-19.pc
endif
if WITH_BACKUP
pkgconfig_DATA += libbarrybackup-19.pc
endif
if WITH_ALX
pkgconfig_DATA += libbarryalx-19.pc
endif

VERSIONED_INCLUDE = barry@BARRY_MAJOR@

all-local:
	rm -f $(top_builddir)/barry $(top_builddir)/$(VERSIONED_INCLUDE)
	ln -fs $(top_srcdir)/src $(top_builddir)/barry
	ln -fs $(top_builddir) $(top_builddir)/$(VERSIONED_INCLUDE)

clean-local:
	rm -f $(top_builddir)/barry $(top_builddir)/$(VERSIONED_INCLUDE)

# these directories will be tested for during make dist to warn if they are
# missing - a new nested package should be added both here and in configure.ac
CONDITIONAL_SUBDIRS = gui opensync-plugin opensync-plugin-0.4x desktop
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

