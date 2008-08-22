#!/bin/sh

# Meant to be run from the root of a Barry source tree.

sed  "1 s/(.*)/($1.$2-cvs$(date '+%Y%m%d'))/" < debian/changelog > debian/changelog.2
mv debian/changelog.2 debian/changelog

echo "Updated debian/changelog to current date:"
head -n 1 < debian/changelog

