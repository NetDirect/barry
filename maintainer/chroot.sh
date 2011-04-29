#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" -o -z "$5" -o -z "$6" ] ; then
	echo
	echo "Usage: ./chroot.sh user chroot_target files results_from results_to chown_user [args...]"
	echo
	echo "user is the chroot system username to use for building"
	echo
	echo "chroot_target is the path on the host system where the"
	echo "chroot system resides.  example: /var/chroot/ubuntu804"
	echo
	echo "files is a string of possibly multiple files to copy over"
	echo "to the chroot system.  The files will be copied to the"
	echo "/home/user/barrychroot directory and any arguments will be"
	echo "run from there.  Normally, the tarball and the script"
	echo "will be included in this list at a minimum."
	echo
	echo "results_from is the directory where the results will be found"
	echo "after running the arguments.  i.e. if you run make-deb.sh"
	echo "on the chroot system, results_from is the directory where"
	echo "the resulting binary packages can be found.  This path is"
	echo "a local path, and must therefore include chroot_target."
	echo "For example, /var/chroot/ubuntu1004/home/cdfrey/barrychroot/results"
	echo
	echo "results_to is the local directory where the resulting files"
	echo "will be copied to.  The directory will be created if it"
	echo "does not already exist, and it and its contents will be"
	echo "chowned to chown_user."
	echo
	echo "chown_user is the user to chown the resulting files to, on"
	echo "the local system."
	echo
	echo "Expects to run as root."
	echo
	exit 1
fi

CHROOTUSER="$1"
TARGET="$2"
FILES="$3"
RESULTS_FROM="$4"
RESULTS_TO="$5"
CHOWNUSER="$6"
shift 6

set -e

CHROOTDIR="$TARGET/home/$CHROOTUSER/barrychroot"

# create target work directory
rm -rf "$CHROOTDIR"
chroot "$TARGET" su - "$CHROOTUSER" -c /bin/sh -lc \
	"mkdir -p /home/$CHROOTUSER/barrychroot"

# copy source tarball and script to chroot system
cp $FILES "$CHROOTDIR"

# create a quoted argument string
QUOTEDARGS="$1"
shift
for arg ; do
	QUOTEDARGS="$QUOTEDARGS '$arg'"
done

# run the args
chroot "$TARGET" su - "$CHROOTUSER" -c /bin/sh -lc \
	"cd /home/$CHROOTUSER/barrychroot && $QUOTEDARGS"

# copy the results back to the target directory
mkdir -p "$RESULTS_TO"
cp "$RESULTS_FROM"/* "$RESULTS_TO"
chown -R "$CHOWNUSER" "$RESULTS_TO"

# cleanup source tarball and script
rm -rf "$CHROOTDIR"

