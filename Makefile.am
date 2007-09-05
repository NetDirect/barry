
EXTRA_DIST = COPYING ChangeLog README \
	buildgen.sh \
	data \
	doc \
	hotplug \
	maintainer \
	rpm \
	udev

SUBDIRS = . src tools examples man
# conditionally configured nested subdirectories are listed in $(subdirs)
SUBDIRS += $(subdirs)

pkgconfig_DATA = libbarry-0.pc

all-local:
	rm -f $(top_builddir)/barry
	ln -s $(top_srcdir)/src $(top_builddir)/barry

clean-local:
	rm -f $(top_builddir)/barry

# these directories will be tested for during make dist to warn if they are
# missing - a new nested package should be added both here and in configure.ac
CONDITIONAL_SUBDIRS = gui opensync-plugin
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

