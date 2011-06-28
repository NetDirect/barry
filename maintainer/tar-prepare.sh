#!/bin/bash

if [ -n "$1" ] ; then
	echo
	echo "Usage: ./tar-prepare.sh"
	echo
	echo "This script prepares a Barry directory for release, assuming"
	echo "that it is run from inside the root of a freshly extracted"
	echo "Barry source tree."
	echo
	exit 1
fi

set -e

# Are we in a Barry root tree?
if ! [ -f AUTHORS -a -f ChangeLog -a -f COPYING -a -f buildgen.sh ] ; then
	echo "Can't find Barry files. Please run from root of Barry tree."
	exit 1
fi

# Generate web docs
(cd doc/www && ./static.sh)

# Generate doxygen docs
# These are large... let the user run if needed,
# and post on the website instead of in the tarball.
#(doxygen && rm -f doxygen.log)

# Generate configure scripts
./buildgen.sh

# Clean up the extraneous autoconf clutter
rm -rf \
	autom4te.cache \
	gui/autom4te.cache \
	opensync-plugin/autom4te.cache \
	opensync-plugin-0.4x/autom4te.cache \
	desktop/autom4te.cache

