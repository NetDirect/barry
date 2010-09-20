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

echo "Updating barry GIT"
[ -d $GIT_DIR ] && (cd $GIT_DIR && git pull origin) || (git clone $GIT_URL $GIT_DIR)

# Get a copy of the code, update sources ONLY when we have new code.
cd $GIT_DIR
GIT_VER=`git log --pretty=format:'' | wc -l`
if [ ! -f ../${PROJECT}_$VERSION-$GIT_VER.orig.tar.gz ]; then
    echo "Building a new barry upstream tarball."
    git archive --prefix=$PROJECT.tmp/ master > ../$PROJECT.tar
    echo $VERSION > ../$PROJECT.version
    echo $GIT_VER >> ../$PROJECT.version
    cd ../
    (tar xf $PROJECT.tar && rm $PROJECT.tar)

    # Sort out the opensync plugins
    [ -d opensync-plugin-barry.tmp ] && rm -fr opensync-plugin-barry.tmp
    [ -d opensync-plugin-barry-4x.tmp ] && rm -fr opensync-plugin-barry-4x.tmp
    mv $PROJECT.tmp/opensync-plugin opensync-plugin-barry.tmp
    mv $PROJECT.tmp/opensync-plugin-0.4x opensync-plugin-barry-4x.tmp
    cp $PROJECT.version opensync-plugin-barry.version
    cp $PROJECT.version opensync-plugin-barry-4x.version
else
    echo "No new Version of barry: Skipping"
    cd ../
fi

# Prepare the opensync svn too
VERSION=0.39
SVN_DIR=opensync-svn
SVN_URL=http://svn.opensync.org/trunk
SVN_PAC=libopensync-0.40-snapshot
echo "Updating opensync SVN"
[ -d $SVN_DIR ] && (cd $SVN_DIR && svn update &> /dev/null) || (svn checkout $SVN_URL $SVN_DIR)
cd $SVN_DIR
SVN_VER=`svnversion .`
if [ ! -f ../opensync_$VERSION-$SVN_VER.orig.tar.gz ]; then
    echo "Building a new OpenSync upstream tarball"
    mkdir build; cd build
    cmake ../ &> cmake.log
    make package_source &> package.log
    tar xjf $SVN_PAC.tar.bz2
    mv $SVN_PAC ../../opensync.tmp
    cd ../
    rm -fr build
    echo $VERSION > ../opensync.version
    echo $SVN_VER >> ../opensync.version
else
    echo "No New Version of OpenSync: Skipping"
fi
cd ../

for PACKAGE in barry opensync opensync-plugin-barry opensync-plugin-barry-4x; do
    VERSION=`sed -n '1p' $PACKAGE.version`
    REVISION=`sed -n '2p' $PACKAGE.version`
    DIR=$PACKAGE-$VERSION
    echo "Packaging $PACKAGE-$VERSION (R$REVISION)"
    [ -d $DIR ] && rm -fr $DIR

    # New build of sources
    if [ ! -f ${PACKAGE}_$VERSION-$REVISION.orig.tar.gz ]; then
        # Skip if we don't have something to build
        if [ ! -d $PACKAGE.tmp ]; then
            continue
        fi
        mv $PACKAGE.tmp $DIR
        tar czf ${PACKAGE}_$VERSION-$REVISION.orig.tar.gz $DIR
    else
        # Existing build of sources
        tar xzf ${PACKAGE}_$VERSION-$REVISION.orig.tar.gz
    fi

    if [ -d debian-$PACKAGE ]; then
        cp -R debian-$PACKAGE $DIR/debian
    fi

    for RELEASE in karmic lucid maverick; do
        # Make a vew version for us.
        MAJOR=0
        DEBVER="$VERSION-$REVISION-${MAJOR}${RELEASE}0"
        while [ -f ${PACKAGE}_${DEBVER}_source.changes ]; do
            if [ -f ${PACKAGE}_${DEBVER}.dsc ]; then
                echo " > Skip $RELEASE"
                continue 2
            fi
            let MAJOR++
            DEBVER="$VERSION-$REVISION-${MAJOR}${RELEASE}0"
        done
        echo "  > Build $MAJOR for $RELEASE"
        cd $DIR
        rm debian/changelog
        # Generate a new changelog file
        dch --create --package=$PACKAGE -v $DEBVER --distribution $RELEASE "GIT Build for $PPA"
        # Build our debian source package <- This assumes gpg-agent
        debuild -S &> ../$PACKAGE-$RELEASE.log
        # Put our debian source package into launchpad
        cd ../
        dput $PPA ${PACKAGE}_${DEBVER}_source.changes
    done;
done;



