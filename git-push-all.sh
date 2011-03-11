#!/bin/sh

PATCH_CVS="$(dirname "$0")/git-patch-cvs.sh"

BRANCHES="master scripts pristine-tar v0.17.2"

set -e

# Note: This push only updates non-CVS branches, and master.
#       To push other CVS branches, do it manually, with git-patch-cvs.sh

git push -f --tags origin $BRANCHES
git push -f --tags nfshome $BRANCHES

echo "Press enter when ready for git push sourceforge.net..."
read
git push sourceforge.net $BRANCHES


echo "Press enter when ready for git push repo.or.cz..."
read
#
# Now update repo.or.cz.  We can be more liberal here, since repo.or.cz
# _should_ contain everything that can be considered public.
#
# The idea is to put everything out there, in case someone wants
# to work on it, or send patches.
#
# "all" and "tags" have to be done separately for some reason
#
#git push --all repo.or.cz
#git push --tags repo.or.cz master i18n
git push repo.or.cz $BRANCHES

echo "================= NOTE: if you want to push a tag, do it manually"
echo "=================       for both repo.or.cz and sourceforge.net!"
echo "Example: git push repo.or.cz barry-0.13"

