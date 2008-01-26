///
/// \file	pppob.cc
///		In the same vein as pppoe, used with pppd to create a
///		pty tunnel and GPRS modem link.
///

/*
    Copyright (C) 2007-2008, Net Direct Inc. (http://www.netdirect.ca/)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/

#include <barry/barry.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <getopt.h>
#include <pthread.h>


using namespace std;
using namespace Barry;

volatile bool read_finished = false;
bool data_dump = false;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "pppob - PPP over Barry\n"
   << "        Copyright 2007-2008, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device plugged in, this flag is optional\n"
   << "   -v        Dump protocol data during operation (debugging only!)\n"
   << endl;
}

void *UsbReadThread(void *userptr)
{
	SocketRoutingQueue *q = (SocketRoutingQueue *)userptr;

	// read from USB and write to stdout until finished
	while( !read_finished ) {
		q->DoRead(2000);	// FIXME - timeout in milliseconds?
	}
}

void SerialDataCallback(Data *data)
{
	int todo = data.GetSize();
	const unsigned char *buf = data.GetData();

	if( todo && data_dump )
		cerr << "ReadThread:\n" << data << endl;

	while( todo ) {
		int written = write(1, buf, todo);
		if( written > 0 ) {
			todo -= written;
			buf += written;
		}
		else {
			cerr << "Error in write()" << endl;
		}
	}
}

int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "p:v");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'v':	// data dump on
				data_dump = true;
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump);

		// Probe the USB bus for Blackberry devices and display.
		// If user has specified a PIN, search for it in the
		// available device list here as well
		Barry::Probe probe;
		int activeDevice = probe.FindActive(pin);
		if( activeDevice == -1 ) {
			if( pin )
				cerr << "PIN " << setbase(16) << pin
					<< " not found" << endl;
			cerr << "No device selected" << endl;
			return 1;
		}

		// Create our socket router
		SocketRoutingQueue router;

		// Start USB read thread, to handle all routing
		pthread_t usb_read_thread;
		if( pthread_create(&usb_read_thread, NULL, UsbReadThread, &router) ) {
			cerr << "Error creating USB read thread." << endl;
			return 1;
		}

		// Create our controller object
		Controller con(probe.Get(activeDevice), router);

		// Open serial mode... the callback handles reading from
		// USB and writing to stdout
		Mode::Serial serial(con, SerialDataCallback);

		// Read from stdin and write to USB, until
		// stdin is closed
		Data data;
		int bytes_read;
		while( (bytes_read = read(0, data.GetBuffer(), data.GetBufSize())) != 0 ) {
			if( bytes_read > 0 ) {
				data.ReleaseBuffer(bytes_read);
				con.SerialWrite(data); // FIXME - does this copy data?
			}
		}


		// flag end / kill read thread
		read_finished = true;
		pthread_join(usb_read_thread, NULL);

	}
	catch( std::exception &e ) {
		cerr << "exception caught in main(): " << e.what() << endl;
		return 1;
	}

	return 0;
}

