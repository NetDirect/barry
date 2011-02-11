#!/bin/sh

# meant to be run from inside a freshly checked out cvs workspace
# builds: library, gui, then both plugin

set -e

SCRIPTDIR="$(dirname "$0")"

./buildgen.sh

"$SCRIPTDIR"/configure-barry.sh
make -j2
make install

