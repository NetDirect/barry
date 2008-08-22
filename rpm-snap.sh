#!/bin/sh

# Meant to be run from the root of a Barry source tree.

sed  "/^Release: / s/: .*$/: $1.$2-cvs$(date '+%Y%m%d')/" < rpm/barry.spec > rpm/barry.spec.2
mv rpm/barry.spec.2 rpm/barry.spec

echo "Updated rpm/barry.spec to current date:"
grep "^Release: " rpm/barry.spec

