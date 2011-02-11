#!/bin/sh

# meant to be run from inside a freshly checked out cvs workspace
# and run after the base library is compiled.

set -e

SCRIPTDIR="$(dirname "$0")"

cd opensync-plugin
../"$SCRIPTDIR"/configure-barryopensync.sh
make -j2
make install

