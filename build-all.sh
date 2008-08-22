#!/bin/sh

# meant to be run from inside a freshly checked out cvs workspace
# builds: library, gui, then plugin

set -e

SCRIPTDIR="$(dirname "$0")"

./buildgen.sh
"$SCRIPTDIR"/configure-barry.sh
make
make install

cd gui
../"$SCRIPTDIR"/configure-barrygui.sh
make
make install

cd ../opensync-plugin
../"$SCRIPTDIR"/configure-barryopensync.sh
make
make install

