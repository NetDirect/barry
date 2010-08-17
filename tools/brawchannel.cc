///
/// \file	brawchannel.cc
///
///

/*
    Copyright (C) 2010, RealVNC Ltd.

        Some parts are inspired from bjavaloader.cc

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
#include <cstdio>
#include <algorithm>
#include <getopt.h>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "i18n.h"

using namespace std;
using namespace Barry;

class StdoutWriter : public Barry::Mode::RawChannelDataCallback
{
public:
	StdoutWriter(bool& keepGoing, bool verbose)
		: m_continuePtr(&keepGoing)
		, m_verbose(verbose)
		{
		}

    
	void DataReceived(Data& data)
		{
			if (m_verbose)
			{
				std::cerr << "From BB: ";
				data.DumpHex(std::cerr);
				std::cerr << "\n";
			}

			size_t toWrite = data.GetSize();
			size_t written = 0;

			while (written < toWrite && *m_continuePtr)
			{
				ssize_t writtenThisTime = write(STDOUT_FILENO, &(data.GetData()[written]), toWrite - written);
				if (m_verbose)
					cerr << "Written " << writtenThisTime << " bytes over stdout\n";
				std::fflush(stdout);
				if (writtenThisTime < 0)
				{
					*m_continuePtr = false;
				}
				else
				{
					written += writtenThisTime;
				}
			}
		}

private:
	bool* m_continuePtr;
	bool m_verbose;
};

void Usage()
{
	int major, minor;
	const char *Version = Barry::Version(major, minor);

	cerr
		<< "brawchannel - Command line USB Blackberry raw channel interface\n"
		<< "        Copyright 2010, RealVNC Ltd.\n"
		<< "        Using: " << Version << "\n"
		<< "\n"
		<< "Usage:\n"
		<< "brawchannel [options] <channel name>\n"
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

	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {
		uint32_t pin = 0;
		bool data_dump = false;
		string password;
		vector<string> params;
		string busname;
		string devname;
		string iconvCharset;
		Usb::EndpointPair epOverride;

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

		if( argc < 1 ) {
			cerr << "Error: Missing raw channel name." << endl;
			Usage();
			return 1;
		}

		// Fetch command from remaining arguments
		string channelName = argv[0];
		argc --;
		argv ++;

		// Put the remaining arguments into an array
		for (; argc > 0; argc --, argv ++) {
			params.push_back(string(argv[0]));
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

		// Now start to read from cin and get ready to write
		// to the BlackBerry.
		if (data_dump)
			std::cerr << "Connected to device, starting read/write\n";

		bool running = true;

		// Create the thing which will write onto stdout
		StdoutWriter stdoutWriter(running, data_dump);

		// Set up the BlackBerry gubbins
		// Start a thread to handle any data arriving from
		// the BlackBerry.
		auto_ptr<SocketRoutingQueue> router;
		router.reset(new SocketRoutingQueue);
		router->SpinoffSimpleReadThread();

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice), *router);
		Barry::Mode::RawChannel rawChannel(con, stdoutWriter);

		//
		// execute each mode that was turned on
		//
		rawChannel.Open(password.c_str(), channelName.c_str());

		// We now have a thread running to read from the
		// BB and write over stdout; in this thread we'll
		// read from stdin and write to the BB.
		unsigned char buf[Barry::Mode::RawChannel::MaximumPacketContentsSize];
		while (running)
		{
			size_t haveRead = read(STDIN_FILENO, buf, sizeof(buf));
			if (haveRead > 0)
			{
				Data toWrite(buf, haveRead);
				if(data_dump)
				{
					std::cerr << "Sending " << haveRead << " bytes stdin->USB\n";
					std::cerr << "To BB: ";
					toWrite.DumpHex(std::cerr);
					std::cerr << "\n";
				}
				rawChannel.Send(toWrite);
				if(data_dump)
					std::cerr << "Sent " << haveRead << " bytes stdin->USB\n";
			}
			else if (haveRead < 0 || !std::cin.good())
			{
				running = false;
			}
		}
	}
	catch( Usb::Error &ue) {
		std::cerr << "Usb::Error caught: " << ue.what() << endl;
		return 1;
	}
	catch( Barry::Error &se ) {
		std::cerr << "Barry::Error caught: " << se.what() << endl;
		return 1;
	}
	catch( std::exception &e ) {
		std::cerr << "std::exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

