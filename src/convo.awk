#
# This script is useful for turning a UsbSnoop log file into something
# more readable.  It assumes USB endpoints 0x05 and 0x82
#

BEGIN {
	#  0 for down
	#  1 for up
	direction = 0;

	# boolean
	docopy = 0;
}

/>>>/ {
	direction = 0;
	docopy = 0;
	print $0;
}

/<<</ {
	direction = 1;
	docopy = 0;
	print $0;
}

/endpoint 0x00000005/ {
	# only copy data going to the output endpoint
	if( direction == 0 ) {
		docopy = 1;
		printf "\nsep: 5\n";
	}
}

/endpoint 0x00000082/ {
	# only copy data coming from the input endpoint
	if( direction == 1 ) {
		docopy = 1;
		printf "rep: 82\n";
	}
}

/^    [0-9AaBbCcDdEeFf]*: / {
	if( docopy ) {
		print $0;
	}
}

