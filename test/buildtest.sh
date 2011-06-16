#!/bin/bash

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
# Check whether the desktop 0.22/0.4x optional build tests can be done
#
DESKTOP_OPTIONAL_BUILD_TEST=1
if pkg-config --list-all |grep opensync ; then
	DESKTOP_OPTIONAL_BUILD_TEST=0
	echo
	echo "A version of opensync is already installed on your"
	echo "system in default directories.  Therefore the"
	echo "tests of the optional desktop builds cannot be done,"
	echo "and will therefore be skipped."
	echo
	echo "Press enter to continue..."
	read
fi

LIBUSB_0_1_OPTIONAL_BUILD_TEST=1
if ! pkg-config --list-all | grep "libusb " ; then
	LIBUSB_0_1_OPTIONAL_BUILD_TEST=0
	echo
	echo "No libusb 0.1 library found in pkgconfig, will"
	echo "not perform libusb 0.1 based tests."
	echo
	echo "Press enter to continue..."
	read
fi

LIBUSB_1_0_OPTIONAL_BUILD_TEST=1
if ! pkg-config --list-all | grep "libusb-1.0 " ; then
	LIBUSB_1_0_OPTIONAL_BUILD_TEST=0
	echo
	echo "No libusb 1.0 library found in pkgconfig, will"
	echo "not perform libusb 1.0 based tests."
	echo
	echo "Press enter to continue..."
	read
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
echo "Moving barry into it's own directory..."
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
./configure --prefix="$BASEPATH/build/rootdir" --disable-sync
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

if [ "$DESKTOP_OPTIONAL_BUILD_TEST" = "1" ] ; then
	BACKUP_PKG_CONFIG_PATH="$PKG_CONFIG_PATH"

	echo "Testing optional desktop builds in 10 seconds..."
	sleep 10s

	cd desktop

	# Test only 0.22
	export PKG_CONFIG_PATH="$BASEPATH/build/rootdir/lib/pkgconfig:$OSYNCROOTDIR/lib/pkgconfig"

	export CXXFLAGS="-Wall -Werror -O0 -g"
	./configure --prefix="$BASEPATH/build/rootdir"
	make $MAKEOPTS
	make install
	make distclean

	# Test only 0.4x
	export PKG_CONFIG_PATH="$BASEPATH/build/rootdir/lib/pkgconfig:$OSYNCROOTDIR_0_40/lib/pkgconfig"

	export CXXFLAGS="-Wall -Werror -O0 -g"
	./configure --prefix="$BASEPATH/build/rootdir"
	make $MAKEOPTS
	make install
	make distclean

	cd ..
	export PKG_CONFIG_PATH="$BACKUP_PKG_CONFIG_PATH"
fi


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
# Test libusb selection from configure
# using with and without selection
#
if [ "$LIBUSB_0_1_OPTIONAL_BUILD_TEST" = "1" -a "$LIBUSB_1_0_OPTIONAL_BUILD_TEST" = "1" ] ; then
    echo "Testing choosing libusb options..."
    # Test that libusb 1.0 is chosen by default
    ./configure --prefix="$BASEPATH/build/rootdir"
    make $MAKEOPTS install
    ldd $BASEPATH/build/rootdir/lib/libbarry.so | grep libusb-1.0.so
    rm -rf "$BASEPATH/build/rootdir"
    make distclean

    # Test that libusb 0.1 is chosen if not libusb 1.0
    ./configure --prefix="$BASEPATH/build/rootdir" \
	--without-libusb1_0
    make $MAKEOPTS install
    ldd $BASEPATH/build/rootdir/lib/libbarry.so | grep libusb-0.1.so
    rm -rf "$BASEPATH/build/rootdir"
    make distclean

    # Test that libusb 0.1 is chosen if explicitly asked for
    ./configure --prefix="$BASEPATH/build/rootdir" \
	--with-libusb
    make $MAKEOPTS install
    ldd $BASEPATH/build/rootdir/lib/libbarry.so | grep libusb-0.1.so
    rm -rf "$BASEPATH/build/rootdir"
    make distclean
fi


#
# Test that cleanall cleans up all traces
#
echo "Testing buildgen cleanall..."
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
mkdir ../disttree
tar -C ../disttree -xjf barry-*.*.*.tar.bz2
make distcheck
make distclean
# remove the dist tarballs
rm -f barry-*.*.*.tar.{bz2,gz}
# compare our tree with the disttree
cd ..
# skip:
#   autom4te.cache - autogenerated stuff we don't care about
#   debian - dist tarball shouldn't have to care about that
#   m4/ - more autogenerated stuff we dont' care about
#   po/ - unsure
#
diff -ruN \
	--exclude=autom4te.cache \
	--exclude=debian \
	--exclude='*.m4' \
	--exclude=po \
	barry disttree/barry-*.*.*

#
# Success
#
cd "$BASEPATH"
rm -rf "$BASEPATH/build"
echo "All tests passed."

