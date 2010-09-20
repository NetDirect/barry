#!/bin/bash
#
# Written by Martin Owens doctormo@gmail.com,
# License: Public Domain
# Revision: 3
#

PROJECT=barry
VERSION=0.17
PPA=ppa:doctormo/barry-snapshot
GIT_DIR=barry-git
GIT_URL=git://barry.git.sourceforge.net/gitroot/barry/barry

[ -d $GIT_DIR ] && (cd $GIT_DIR && git pull origin) || (git clone $GIT_URL $GIT_DIR)

# Get a copy of the code
cd $GIT_DIR
git archive --prefix=$PROJECT.tmp/ master > ../$PROJECT.tar
echo $VERSION > ../$PROJECT.version
git log --pretty=format:'' | wc -l >> ../$PROJECT.version
cd ../
(tar xf $PROJECT.tar && rm $PROJECT.tar)

# Generate autoconf / automake / libtool / autopoint particulars for everything
#(cd $PROJECT.tmp && ./buildgen.sh &> buildgen.log)

# Clean up the extraneous autoconf clutter
#(cd $PROJECT.tmp && \
#	rm -rf \
#		autom4te.cache \
#		gui/autom4te.cache \
#		opensync-plugin/autom4te.cache \
#		opensync-plugin-0.4x/autom4te.cache)

# Sort out the opensync plugins
[ -d opensync-plugin-barry.tmp ] && rm -fr opensync-plugin-barry.tmp
[ -d opensync-plugin-barry-4x.tmp ] && rm -fr opensync-plugin-barry-4x.tmp
mv $PROJECT.tmp/opensync-plugin opensync-plugin-barry.tmp
mv $PROJECT.tmp/opensync-plugin-0.4x opensync-plugin-barry-4x.tmp
cp $PROJECT.version opensync-plugin-barry.version
cp $PROJECT.version opensync-plugin-barry-4x.version

# Prepare the opensync svn too
VERSION=0.39
SVN_DIR=opensync-svn
SVN_URL=http://svn.opensync.org/trunk
SVN_PAC=libopensync-0.40-snapshot
[ -d $SVN_DIR ] && (cd $SVN_DIR && svn update) || (svn checkout $SVN_URL $SVN_DIR)
cd $SVN_DIR
mkdir build; cd build
cmake ../ &> cmake.log
make package_source &> package.log
tar xjf $SVN_PAC.tar.bz2
mv $SVN_PAC ../../opensync.tmp
cd ../
rm -fr build
echo $VERSION > ../opensync.version
svnversion . >> ../opensync.version
cd ../

for PACKAGE in barry opensync opensync-plugin-barry opensync-plugin-barry-4x; do
    VERSION=`sed -n '1p' $PACKAGE.version`
    REVISION=`sed -n '2p' $PACKAGE.version`
    DIR=$PACKAGE-$VERSION
    [ -d $DIR ] && rm -fr $DIR
    # Skip if we don't have something to build
    if [ ! -d $PACKAGE.tmp ]; then
        continue
    fi
    echo "Packaging $PACKAGE-$VERSION (R$REVISION)"
    mv $PACKAGE.tmp $DIR

    tar czf ${PACKAGE}_$VERSION-$REVISION.orig.tar.gz $DIR
    if [ -d debian-$PACKAGE ]; then
        cp -R debian-$PACKAGE $DIR/debian
    fi

    for RELEASE in karmic lucid maverick; do
        echo "  > For $RELEASE"
        # Make a vew version for us.
        MAJOR=0
        DEBVER="$VERSION-$REVISION-${MAJOR}${RELEASE}0"
        while [ -f ${PACKAGE}_${DEBVER}_source.changes ]; do
            let MAJOR++
            DEBVER="$VERSION-$REVISION-${MAJOR}${RELEASE}0"
        done
        cd $DIR
        rm debian/changelog
        # Generate a new changelog file
        dch --create --package=$PACKAGE -v $DEBVER --distribution $RELEASE "GIT Build for $PPA"
        # Build our debian source package
        debuild -S 1> $PACKAGE-$RELEASE.log
        # Put our debian source package into launchpad
        cd ../
        dput $PPA ${PACKAGE}_${DEBVER}_source.changes
    done;
    # Clean up everything
    rm $PACKAGE.version
done;



