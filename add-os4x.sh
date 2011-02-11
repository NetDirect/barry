#!/bin/sh

# meant to be run from inside a freshly checked out cvs workspace
# and run after the base library is compiled.

set -e

SCRIPTDIR="$(dirname "$0")"

cd opensync-plugin-0.4x
../"$SCRIPTDIR"/configure-barryopensync-0.4x.sh
make -j2
make install

