#!/bin/bash

set -e

cd $(dirname "$0")/../maintainer
./release.sh 0 19 0 master barrychroots
./release.sh 0 19 0 master barryremote

cd ../test
git clean -xdf
./buildtest.sh

echo
echo "Success."
echo "Everything built without failure."

