#!/bin/bash

#
# Note that to use this script, you need to add "%_gpg_name" to your
# ~/.rpmmacros file.
#

set -e

# Sign all RPMs found under bmbuild/
rpmsign --addsign $(find bmbuild -name "*.rpm" -print)

# Create the YUM repo files for each architecture
for arch in i386 source-i386 x86_64 source-x86_64 ; do
	for dir in $(find bmbuild -name "$arch" -type d -print) ; do
		createrepo $dir
		cp yum/key $dir/RPM-GPG-KEY-binary-meta
	done
done

