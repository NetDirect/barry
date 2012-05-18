#!/bin/bash

#
# Note that to use this script, you need to add "%_gpg_name" to your
# ~/.rpmmacros file.
#

set -e

if [ -z "$1" ] ; then
	echo "Usage: make-yum.sh builddir"
	exit 1
fi

if [ -z "$GPG_AGENT_INFO" ] ; then
	echo "Press enter to continue if gpg-agent is running,"
	echo "Otherwise, run:"
	echo
	echo "      eval \$(gpg-agent --daemon --default-cache-ttl n)"
	echo
	echo "to preserve your sanity.  n is in seconds, and default"
	echo "from gpg-agent is 600 seconds."
	read
fi

BUILDDIR="$1"

# Sign all RPMs found under $BUILDDIR/
rpmsign --addsign $(find $BUILDDIR -name "*.rpm" -print)

# Create the YUM repo files for each architecture
for arch in i386 source-i386 i586 source-i586 i686 source-i686 x86_64 source-x86_64 ; do
	for dir in $(find $BUILDDIR -name "$arch" -type d -print) ; do
		createrepo $dir
		cp yum/key $dir/RPM-GPG-KEY-barry
	done
done

# Sign the repomd.xml files (opensuse checks for this)
for rfile in $(find $BUILDDIR -type f -name "repomd.xml" -print) ; do
	gpg --use-agent --default-key B6C2250E --armor \
		--output $rfile.asc \
		--detach-sign $rfile
	cp yum/key $rfile.key
done

