#!/bin/sh

# meant to be run from inside a freshly checked out cvs workspace
# builds: library, gui, then both plugin

set -e

SCRIPTDIR="$(dirname "$0")"

./buildgen.sh

"$SCRIPTDIR"/configure-barry.sh
make -j2
make install

cd gui
../"$SCRIPTDIR"/configure-barrygui.sh
make -j2
make install

cd ../opensync-plugin
../"$SCRIPTDIR"/configure-barryopensync.sh
make -j2
make install

cd ../opensync-plugin-0.4x
../"$SCRIPTDIR"/configure-barryopensync-0.4x.sh
make -j2
make install

cd ../desktop
../"$SCRIPTDIR"/configure-barrydesktop.sh
make -j2
make install

