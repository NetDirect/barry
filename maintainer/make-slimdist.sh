#!/bin/bash

#
# Creates a tarball of just the repository metadata, not the binary
# packages themselves.
#

tar --exclude=*.deb --exclude=*.rpm -cvz -C bmbuild -f metadata.tar.gz .

