#!/bin/bash
#
# Written by Martin Owens doctormo@gmail.com,
# License: Public Domain
# Revision: 2
#

VERSION=0.16
RELEASE=jaunty
PPA=ppa:doctormo/barry-snapshot
DIR=barry-$VERSION
GIT_DIR=barry-git
#GIT_URL=git://repo.or.cz/barry.git
GIT_URL=git://barry.git.sourceforge.net/gitroot/barry/barry

[ -d $DIR ] && rm -fr $DIR
[ -d $GIT_DIR ] && (cd $GIT_DIR && git pull origin) || (git clone $GIT_URL $GIT_DIR)

# Get a copy of the code
rm barry_$VERSION.orig.tar.gz
cd $GIT_DIR
git archive --prefix=$DIR/ master > ../barry_$VERSION.orig.tar
cd ../
tar xvf barry_$VERSION.orig.tar
gzip barry_$VERSION.orig.tar

# Go into the code
cd $DIR

# Generate a new changelog file
rm debian/changelog
DATEVER=$VERSION-0git`date +%Y%m%d%S`
dch --create --package=barry -v $DATEVER --distribution $RELEASE "Weekly GIT Build for $PPA"

# Build our debian source package
debuild -S

# Put our debian source package into launchpad
cd ../
dput $PPA barry_${DATEVER}_source.changes

