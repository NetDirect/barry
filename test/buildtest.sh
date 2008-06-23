#!/bin/sh

set -e

Usage() {
	echo "Main Barry build test script."
	echo
	echo "Usage:"
	echo "       ./buildtest.sh /path/to/libopensync-0.22.tar.bz2"
	echo
	echo "Or, write a ~/.barrytest file that contains the directory"
	echo "you used as a --prefix when building your own libopensync."
	echo "Note that this directory must be writable by the user that"
	echo "runs the test, as the plugin will be installed during the"
	echo "build test."
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

# Comment these out if you don't have ccache installed
export CC="ccache gcc"
export CXX="ccache g++"

BASEPATH=$(pwd)
OSYNCSOURCE="$1"

#
# First, build opensync in a local directory
#

if [ -z "$OSYNCSOURCE" -a -f ~/.barrytest ] ; then
	read OSYNCROOTDIR < ~/.barrytest
elif [ -n "$OSYNCSOURCE" ] ; then
	echo "Extracting opensync sources and building..."
	(cd build && tar xjf "$OSYNCSOURCE" && \
		cd libopensync-0.22 && \
		./configure -prefix="$BASEPATH/build/osyncrootdir" && \
		make install)
	OSYNCROOTDIR="$BASEPATH/build/osyncrootdir"
else
	Usage
	exit 1
fi
echo "Using opensync rootdir: $OSYNCROOTDIR"



#
# Move .. barry into its own buildable directory
#

mkdir -p build/barry
(tar -C .. --exclude=CVS --exclude=.git --exclude=test/build -cf - . | \
	tar -C build/barry -xf -)
diff -ruN --exclude=CVS --exclude=.git --exclude=test .. build/barry



#
# Prepare for Barry building
#
cd build/barry
export PKG_CONFIG_PATH="$BASEPATH/build/rootdir/lib/pkgconfig:$OSYNCROOTDIR/lib/pkgconfig"


#
# Create configure script
#
./buildgen.sh


#
# Build and test as individual packages
#
echo "Individual package build test..."

rm -rf "$BASEPATH/build/rootdir"

export CXXFLAGS="-Wall -Werror -pedantic -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make
make distclean
./configure --prefix="$BASEPATH/build/rootdir" --with-boost
make
make install
make distclean

cd gui
export CXXFLAGS="-Wall -Werror -pedantic -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make install
make distclean
cd ..

cd opensync-plugin
export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir"
make install
make distclean
cd ..



#
# Build and test as one package
#

echo "Single build test..."

rm -rf "$BASEPATH/build/rootdir"

export CXXFLAGS="-Wall -Werror -O0 -g"
./configure --prefix="$BASEPATH/build/rootdir" --with-boost \
	--enable-gui --enable-opensync-plugin
make install
make distclean
./configure --prefix="$BASEPATH/build/rootdir" \
	--enable-gui --enable-opensync-plugin
make
make distclean


#
# Test 'make dist'
#
echo "Testing 'make dist'..."

rm -rf "$BASEPATH/build/rootdir"

./configure
make dist
make distcheck
make distclean

./configure --enable-gui --enable-opensync-plugin
make dist
make distcheck
make distclean



#
# Test that cleanall cleans up all traces
#
./buildgen.sh cleanall
cd "$BASEPATH"
diff -ruN --exclude=CVS --exclude=.git --exclude=test .. build/barry


#
# Success
#
rm -rf "$BASEPATH/build"
echo "All tests passed."

