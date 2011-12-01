#!/bin/bash

set -e

if [ -z "$1" -o -z "$2" -o -z "$3" ] ; then
	echo "make-redirect.sh builddir prefix_dir new_url_base"
	echo
	echo "Creates .htaccess output for redirecting all .deb and .rpm files"
	echo "to another website."
	echo
	echo "builddir is where the resulting tree of release-bm.sh is located"
	echo
	echo "prefix_dir is the path from the redirecting site, from the"
	echo "root of the domain name.  Must start with a slash."
	echo "For example: /binary-meta"
	echo
	echo "new_url_base is the base URL of the site where the real files"
	echo "exist.  Do not use a trailing slash, as it will be appended"
	echo "with the filename."
	echo
	echo "Example:"
	echo
	echo "   make-redirect.sh bmbuild /binary-meta http://downloads.sourceforge.net/project/barry/binary-meta/20111020"
	echo
	exit 1
fi

BUILDDIR="$1"
PREFIX_DIR="$2"
NEW_URL_BASE="$3"

# Build Packages and Contents, for all distros, per arch
for file in $(cd $BUILDDIR && find * -type f \( -name "*.rpm" -o -name "*.deb" \) -print) ; do
	echo "Redirect 302 $PREFIX_DIR/$file $NEW_URL_BASE/$file"
done

