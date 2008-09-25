#!/bin/sh

PATCH_CVS="$(dirname "$0")/git-patch-cvs.sh"

set -e

$PATCH_CVS origin master ../barry3
$PATCH_CVS origin master ../external/barry

echo "Press enter when ready for git push..."
read

# Note: This push only updates non-CVS branches, and master.
#       To push other CVS branches, do it manually, with git-patch-cvs.sh

git push --tags origin master i18n scripts pristine-tar


echo "Press enter when ready for git push to repo.or.cz..."
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
git push repo.or.cz master i18n scripts pristine-tar

echo "================= NOTE: if you want to push a tag, do it manually!"
echo "Example: git push repo.or.cz barry-0.13"

