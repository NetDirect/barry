///
/// \file	pppob.cc
///		In the same vein as pppoe, used with pppd to create a
///		pty tunnel and GPRS modem link.
///

/*
    Copyright (C) 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <memory>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "i18n.h"

#include "barrygetopt.h"

using namespace std;
using namespace Barry;

bool data_dump = false;
volatile bool signal_end = false;

void Usage()
{
   int logical, major, minor;
   const char *Version = Barry::Version(logical, major, minor);

   cerr
   << "pppob - PPP over Barry\n"
   << "        Copyright 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -l file   Direct pppob log output to file (useful with -v)\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -s        Use Serial mode instead of IpModem\n"
   << "   -v        Dump protocol data during operation (debugging only!)\n"
   << endl;
}

void signal_handler(int signum)
{
	signal_end = true;
}

void SerialDataCallback(void *context, const unsigned char *data, int len)
{
	if( len && data_dump )
		barryverbose("ReadThread:\n" << Data(data, len));

	while( len ) {
		int written = write(1, data, len);
		if( written > 0 ) {
			len -= written;
			data += written;
		}
		else {
			barryverbose("Error in write()");
		}
	}
}

void ProcessStdin(Modem &modem)
{
	// Read from stdin and write to USB, until
	// stdin is closed
	Data data;
	int bytes_read;
	fd_set rfds;
	struct timeval tv;
	int ret;

	// Handle interrupt signals from pppd
	signal_end = false;
	signal(SIGINT, &signal_handler);
	signal(SIGHUP, &signal_handler);
	signal(SIGTERM, &signal_handler);

	FD_ZERO(&rfds);
	while( signal_end == false ) {
		// Need to use select() here, so that pppd doesn't
		// hang when it tries to set the line discipline
		// on our stdin.

		FD_SET(0, &rfds);
		tv.tv_sec = 30;
		tv.tv_usec = 0;

		ret = select(1, &rfds, NULL, NULL, &tv);
		if( ret == -1 ) {
			perror("select()");
		}
		else if( ret && FD_ISSET(0, &rfds) ) {
			bytes_read = read(0, data.GetBuffer(), data.GetBufSize());
			if( bytes_read == 0 )
				break;

			if( bytes_read > 0 ) {
				data.ReleaseBuffer(bytes_read);
				modem.Write(data);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool force_serial = false;
		std::string logfile;
		std::string password;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "l:p:P:sv");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'l':	// Verbose log file
				logfile = optarg;
				break;

			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'P':	// Device password
				password = optarg;
				break;

			case 's':	// Use Serial mode
				force_serial = true;
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
		// Log to stderr, since stdout is for data in this program.
		std::auto_ptr<std::ofstream> log;
		if( logfile.size() ) {
			log.reset( new std::ofstream(logfile.c_str(), ios::app) );
			Barry::Init(data_dump, log.get());
		}
		else {
			Barry::Init(data_dump, &std::cerr);
		}

		// Display version if in data_dump mode
		if( data_dump ) {
			int logical, major, minor;
			const char *Version = Barry::Version(logical, major, minor);
			barryverbose(Version);
		}

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

		const ProbeResult &device = probe.Get(activeDevice);

		if( !force_serial && device.HasIpModem() ) {
			barryverbose("Using IpModem mode...");

			// Create our controller object using our threaded router.
			Controller con(probe.Get(activeDevice));

			// Open serial mode... the callback handles reading from
			// USB and writing to stdout
			Mode::IpModem modem(con, SerialDataCallback, 0);
			modem.Open(password.c_str());

			ProcessStdin(modem);
			modem.Close();	// graceful close so we can restart without unplugging
		}
		else {
			if( force_serial ) {
				barryverbose("Using Serial mode per command line...");
			}
			else {
				barryverbose("No IpModem mode available, using Serial mode...");
			}

			// Create our socket router and start thread to handle
			// the USB reading, instead of creating our own thread.
			SocketRoutingQueue router;
			router.SpinoffSimpleReadThread();

			// Create our controller object using our threaded router.
			Controller con(probe.Get(activeDevice), router);

			// Open desktop mode... this handles the password side
			// of things
			Mode::Desktop desktop(con);
			desktop.Open(password.c_str());

			// Open serial connection
			Mode::Serial modem(con, SerialDataCallback, 0);
			modem.Open(password.c_str());

			ProcessStdin(modem);
		}

		barryverbose("Exiting");

	}
	catch( std::exception &e ) {
		cerr << "exception caught in main(): " << e.what() << endl;
		return 1;
	}

	return 0;
}

