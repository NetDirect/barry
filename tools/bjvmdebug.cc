///
/// \file	bjvmdebug.cc
///
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <getopt.h>
#include <fstream>
#include <string.h>
#include "i18n.h"

using namespace std;
using namespace Barry;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "bjvmdebug - Command line USB Blackberry Java Debugger\n"
   << "        Copyright 2008-2009, Nicolas VIVIEN.\n"
   << "        Copyright 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -h        This help\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -v        Dump protocol data during operation\n"
   << endl;
}


int main(int argc, char *argv[])
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

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump);

		// Probe the USB bus for Blackberry devices and display.
		// If user has specified a PIN, search for it in the
		// available device list here as well
		Barry::Probe probe;
		int activeDevice = probe.FindActive(pin);
		if( activeDevice == -1 ) {
			cerr << "No device selected, or PIN not found" << endl;
			return 1;
		}

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice));
		Barry::Mode::JVMDebug jvmdebug(con);

		//
		// execute each mode that was turned on
		//
		jvmdebug.Open(password.c_str());
		jvmdebug.Attach();

		// ...Unit tests...
		jvmdebug.Unknown01();
		jvmdebug.Unknown02();
		jvmdebug.Unknown03();
		jvmdebug.Unknown04();
		jvmdebug.Unknown05();

		{
			cout << "Java Modules List :" << endl;
			JVMModulesList list;
			jvmdebug.GetModulesList(list);
			cout << list;
		}

		{
			cout << "Java Threads currently running :" << endl;
			JVMThreadsList list;
			jvmdebug.GetThreadsList(list);
			cout << list;
		}

		jvmdebug.Unknown06();
		jvmdebug.Unknown07();
		jvmdebug.Unknown08();
		jvmdebug.Unknown09();
		jvmdebug.Unknown10();

		jvmdebug.Go();

		for (int i=0; i<20; i++) {
			int ret;
			string msg;
			ret = jvmdebug.GetConsoleMessage(msg);

			if (ret < 0) {
				int status;
				jvmdebug.GetStatus(status);
			}
			else {
				cout << "JVM message : " << msg << endl;
			}
		}

		// End of session
		jvmdebug.Detach();
	}
	catch( Usb::Error &ue) {
		std::cout << endl;	// flush any normal output first
		std::cerr << "Usb::Error caught: " << ue.what() << endl;
		return 1;
	}
	catch( Barry::Error &se ) {
		std::cout << endl;
		std::cerr << "Barry::Error caught: " << se.what() << endl;
		return 1;
	}
	catch( std::exception &e ) {
		std::cout << endl;
		std::cerr << "std::exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

