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
#GIT_URL=git://repo.or.cz/barry.git
GIT_URL=git://barry.git.sourceforge.net/gitroot/barry/barry
DATEVER=$VERSION-`date +%Y%m%d%S`

[ -d $GIT_DIR ] && (cd $GIT_DIR && git pull origin) || (git clone $GIT_URL $GIT_DIR)

# Get a copy of the code
cd $GIT_DIR
git archive --prefix=$PROJECT.tmp/ master > ../barry_$DATEVER.orig.tar
cd ../
tar xf barry_$DATEVER.orig.tar
rm barry_$DATEVER.orig.tar

# Generate autoconf / automake / libtool / autopoint particulars for everything
(cd $PROJECT.tmp && ./buildgen.sh)

# Sort out the opensync plugins
[ -d opensync-plugin-barry.tmp ] && rm -fr opensync-plugin-barry.tmp
[ -d opensync-plugin-barry-4x.tmp ] && rm -fr opensync-plugin-barry-4x.tmp
mv $PROJECT.tmp/opensync-plugin opensync-plugin-barry.tmp
mv $PROJECT.tmp/opensync-plugin-0.4x opensync-plugin-barry-4x.tmp

for PACKAGE in barry opensync-plugin-barry opensync-plugin-barry-4x; do
    echo "Packaging $PACKAGE $DATEVER"
    DIR=$PACKAGE-$VERSION
    [ -d $DIR ] && rm -fr $DIR
    mv $PACKAGE.tmp $DIR

    tar czf ${PACKAGE}_$DATEVER.orig.tar.gz $DIR

    for RELEASE in karmic lucid maverick; do
        echo "  > For $RELEASE"
        cd $DIR
        # Make a vew version for us.
        DEBVER="$DATEVER-0${RELEASE}1"
        rm debian/changelog
        # Generate a new changelog file
        dch --create --package=$PACKAGE -v $DEBVER --distribution $RELEASE "GIT Build for $PPA"
        # Build our debian source package
        debuild -S 1> $PACKAGE-$RELEASE.log
        # Put our debian source package into launchpad
        cd ../
        dput $PPA ${PACKAGE}_${DEBVER}_source.changes
    done;
done;



