#!/bin/sh

#
# Edit these settings to reflect your system
#
MAKEOPTS=-j2
export CC="ccache gcc"
export CXX="ccache g++"

# Make sure any errors stop the test
set -e

Usage() {
	echo "Main Barry build test script."
	echo
	echo "Usage:"
	echo "       ./buildtest.sh /path/to/libopensync-0.22.tar.bz2"
	echo
	echo "Or, write a ~/.barrytest2 file that contains shell commands"
	echo "setting OSYNCROOTDIR to the directory you used as a --prefix"
	echo "when building your own libopensync."
	echo
	echo "Note that this directory must be writable by the user that"
	echo "runs the test, as the plugin will be installed during the"
	echo "build test."
	echo
	echo "You can also set OSYNCROOTDIR_0_40 to the 0.4x opensync directory."
	echo
}

if [ "$1" = "-h" ] ; then
	Usage
	exit 1
fi

#
# Jump to directory that script is located, if necessary
#

if [ "$(dirname $0)" != "." ] ; then
	cd "$(pwd)/$(dirname $0)"
	echo "You ran the script outside the directory it is in."
	echo "Changing directory to: $(pwd)"
	echo "If this is not desired, abort now..."
	sleep 1s
fi

if [ -d "build" ] ; then
	echo "'build' directory already exists, exiting..."
	exit 1
fi

BASEPATH=$(pwd)
OSYNCSOURCE="$1"

mkdir -p build

#
# Do we have a ~/.barrytest2 config?
#
if [ -f ~/.barrytest2 ] ; then
	. ~/.barrytest2
fi


#
# First, build opensync in a local directory
#

if [ -n "$OSYNCROOTDIR" ] ; then
	echo "Using opensync rootdir: $OSYNCROOTDIR"
elif [ -z "$OSYNCROOTDIR" -a -n "$OSYNCSOURCE" ] ; then
	echo "Extracting opensync sources and building..."
	(cd build && tar xjf "$OSYNCSOURCE" && \
		cd libopensync-0.22 && \
		./configure -prefix="$BASEPATH/build/osyncrootdir" && \
		make $MAKEOPTS install)
	OSYNCROOTDIR="$BASEPATH/build/osyncrootdir"
else
	Usage
	exit 1
fi

if [ -n "$OSYNCROOTDIR_0_40" ] ; then
	echo "Using opensync-0.4x rootdir: $OSYNCROOTDIR_0_40"
fi



#
# Move .. barry into its own buildable directory
#

mkdir -p build/barry
(tar -C .. --exclude=CVS --exclude=.git --exclude=test/build -cf - . | \
	tar -C build/barry -xf -)
diff -ruN --exclude=CVS --exclude=.git --exclude=test --exclude=build .. build/barry



#
# Prepare for Barry building
#
cd build/barry
export PKG_CONFIG_PATH="$BASEPATH/build/rootdir/lib/pkgconfig:$OSYNCROOTDIR/lib/pkgconfig:$OSYNCROOTDIR_0_40/lib/pkgconfig"


#
# Create configure script
#
./buildgen.sh


#
# Build and test as individual packages
#
echo "Individual package build test..."

rm -rf "$BASEPATH/build/rootdir"

export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir" --disable-boost
make $MAKEOPTS
make distclean
./configure --prefix="$BASEPATH/build/rootdir" --enable-boost --with-zlib
make $MAKEOPTS
make install
make distclean

cd gui
export CXXFLAGS="-Wall -Werror -ansi -pedantic -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make $MAKEOPTS
make install
make distclean
cd ..

cd opensync-plugin
export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make $MAKEOPTS
make install
make distclean
cd ..

cd opensync-plugin-0.4x
export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make $MAKEOPTS
make install
make distclean
cd ..

cd desktop
export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make $MAKEOPTS
make install
make distclean
cd ..



#
# Build and test as one package
#

echo "Single build test..."

rm -rf "$BASEPATH/build/rootdir"

export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir" \
	--enable-boost \
	--enable-gui \
	--enable-opensync-plugin \
	--enable-opensync-plugin-4x \
	--enable-desktop
make $MAKEOPTS install
make distclean
./configure --prefix="$BASEPATH/build/rootdir" \
	--enable-gui \
	--enable-opensync-plugin \
	--enable-opensync-plugin-4x \
	--enable-desktop
make $MAKEOPTS
make distclean


#
# Test that cleanall cleans up all traces
#
./buildgen.sh cleanall
cd "$BASEPATH"
diff -ruN --exclude=CVS --exclude=.git --exclude=test --exclude=build .. build/barry
cd build/barry



#
# Test 'make dist' (the dist family of targets leaves build evidence
# behind, so do this after the cleanall check)
#
echo "Testing 'make dist'..."

rm -rf "$BASEPATH/build/rootdir"

./buildgen.sh
./configure \
	--enable-gui \
	--enable-opensync-plugin \
	--enable-opensync-plugin-4x \
	--enable-desktop
make dist
make distcheck
make distclean



#
# Success
#
cd "$BASEPATH"
rm -rf "$BASEPATH/build"
echo "All tests passed."

