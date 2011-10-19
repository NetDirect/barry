#!/bin/bash

set -e

echo "Press enter to continue if gpg-agent is running,"
echo "Otherwise, run:"
echo
echo "      eval \$(gpg-agent --daemon)"
echo
echo "to preserve your sanity."
read

# Build Packages and Contents, for all distros, per arch
for arch in i386 amd64 ; do
	for dir in $(find build -name "binary-$arch" -type d -print) ; do
		(cd $dir && \
		apt-ftparchive packages . | gzip -9c > Packages.gz && \
		apt-ftparchive contents . | gzip -9c > ../../Contents-$arch.gz)
	done
done

# Build signed Release files for all debian distros
# Make sure you have gpg-agent running, or this will be a pain...
cd build
for dir in * ; do
	if [ -f ../apt/$dir.conf ] ; then
		(cd $dir && apt-ftparchive -c ../../apt/$dir.conf release . > Release)
		gpg --use-agent --default-key B6C2250E --armor \
			--output $dir/Release.gpg \
			--detach-sign $dir/Release
	fi
done

