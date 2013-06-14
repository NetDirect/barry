///
/// \file	bjdwp.cc
///		bjdwp command line tool
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include "i18n.h"

using namespace std;
using namespace Barry;


void printMessage(const std::string &message);


void Usage()
{
   int logical, major, minor;
   const char *Version = Barry::Version(logical, major, minor);

   cerr << string_vprintf(
   _("bjdwp - Command line USB Blackberry JDWP\n"
   "        Copyright 2008-2009, Nicolas VIVIEN.\n"
   "        Using: %s\n"
   "\n"
   "   -h        This help\n"
   "   -p pin    PIN of device to talk with\n"
   "             If only one device is plugged in, this flag is optional\n"
   "   -P pass   Simplistic method to specify device password\n"
   "   -v        Dump protocol data during operation\n"
   "\n"
   "arguments\n"
   "\n"
   "  <address>  Interface\n"
   "  <port>     Listen port\n"), Version)
   << endl;
}


int main(int argc, char *argv[], char *envp[])
{
	INIT_I18N(PACKAGE);

	try {
		uint32_t pin = 0;
		bool data_dump = false;
		string password;
		vector<string> params;
		string iconvCharset;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "hp:P:v");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'P':	// Device password
				password = optarg;
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

		argc -= optind;
		argv += optind;

		if( argc != 2 ) {
			cerr << _("missing command") << endl;
			Usage();
			return 1;
		}

		// Fetch address & port arguments
		char *address = argv[0];
		int port = atoi(argv[1]);


		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump);

		// Probe the USB bus for Blackberry devices and display.
		// If user has specified a PIN, search for it in the
		// available device list here as well
		Barry::Probe probe;
		int activeDevice = probe.FindActive(pin);
		if( activeDevice == -1 ) {
			cerr << _("No device selected, or PIN not found")
				<< endl;
			return 1;
		}

		Barry::Controller con(probe.Get(activeDevice));
		Barry::Mode::JVMDebug jvmdebug(con);

		// Start JDW daemon...
		//---------------------

		// Create JDWP server and configure
		JDWP::JDWServer server(jvmdebug, address, port);

		// Link device
		server.SetPasswordDevice(password);

		// Redirect console message
		server.SetConsoleCallback(&printMessage);

		server.Start();

		// FIXME - is this needed... couldn't we do a join here?
		while (true)
			sleep(1);

		server.Stop();
	}
	catch( Usb::Error &ue) {
		std::cout << endl;	// flush any normal output first
		std::cerr << _("Usb::Error caught: ") << ue.what() << endl;
		return 1;
	}
	catch( Barry::Error &se ) {
		std::cout << endl;
		std::cerr << _("Barry::Error caught: ") << se.what() << endl;
		return 1;
	}
	catch( std::exception &e ) {
		std::cout << endl;
		std::cerr << _("std::exception caught: ") << e.what() << endl;
		return 1;
	}

	return 0;
}


void printMessage(const std::string &message)
{
	const char esc = 27;
	const int green = 32;
	const int blue = 34;

	std::cout << esc << '[' << green << "mJVM>" << esc << '[' << blue << "m " << message << esc << "[0m";
}

