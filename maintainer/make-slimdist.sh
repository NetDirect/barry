#!/bin/bash

#
# Creates a tarball of just the repository metadata, not the binary
# packages themselves.
#

tar --exclude=*.deb --exclude=*.rpm -cvz -C build -f metadata.tar.gz .

