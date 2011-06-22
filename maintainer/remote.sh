#!/bin/bash

if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" -o -z "$5" -o -z "$6" ] ; then
	echo
	echo "Usage: ./remote.sh user host port files results_from results_to [args...]"
	echo
	echo "user is the username on the remote system to use for building"
	echo
	echo "host is the ssh hostname to build the packages"
	echo
	echo "port is the ssh port on the remote system"
	echo
	echo "files is a string of possibly multiple files to copy over"
	echo "to the remote system.  The files will be copied to the"
	echo "/home/user/barrychroot directory and any arguments will be"
	echo "run from there.  Normally, the tarball and the script"
	echo "will be included in this list at a minimum."
	echo
	echo "results_from is the directory on the remote system where the"
	echo "results will be found after running the arguments.  i.e. if you"
	echo "run make-deb.sh on the remote system, results_from is the"
	echo "directory where the resulting binary packages can be found."
	echo "This path is a remote path and does not include the hostname."
	echo "For example: /home/user/results"
	echo
	echo "results_to is the local directory into which the resulting files"
	echo "will be copied.  The directory will be created if it"
	echo "does not already exist."
	echo
	echo "Expects to be run as a normal user."
	echo
	exit 1
fi

REMOTEUSER="$1"
HOST="$2"
HOSTPORT="$3"
FILES="$4"
RESULTS_FROM="$5"
RESULTS_TO="$6"
shift 6

set -e

function RemoteRun() {
	ssh -x -2 -p $HOSTPORT $REMOTEUSER@$HOST "$1"
}

REMOTEDIR="/home/$REMOTEUSER/barryremote"

# create target work directory
RemoteRun "rm -rf '$REMOTEDIR'"
RemoteRun "mkdir -p '$REMOTEDIR'"

# copy source tarball and script to chroot system
scp -P $HOSTPORT $FILES "$REMOTEUSER@$HOST:$REMOTEDIR"

# create a quoted argument string
QUOTEDARGS="$1"
shift
for arg ; do
	QUOTEDARGS="$QUOTEDARGS '$arg'"
done

# run the args
RemoteRun "cd '$REMOTEDIR' && $QUOTEDARGS"

# copy the results back to the target directory
mkdir -p "$RESULTS_TO"
scp -P $HOSTPORT "$REMOTEUSER@$HOST:$RESULTS_FROM/*" "$RESULTS_TO"

# cleanup source tarball and script
RemoteRun "rm -rf '$REMOTEDIR'"

