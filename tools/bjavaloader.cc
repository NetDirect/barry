///
/// \file	bjavaloader.cc
///		
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

        Some parts are inspired from btool.cc

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
#include <barry/cod.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>


using namespace std;
using namespace Barry;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "bjavaloader - Command line USB Blackberry Java Loader\n"
   << "        Copyright 2008-2009, Nicolas VIVIEN.\n"
   << "        Copyright 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -B bus    Specify which USB bus to search on\n"
   << "   -N dev    Specify which system device, using system specific string\n"
   << "\n"
   << "   -e epp    Override endpoint pair detection.  'epp' is a single\n"
   << "             string separated by a comma, holding the read,write\n"
   << "             endpoint pair.  Example: -e 83,5\n"
   << "             Note: Endpoints are specified in hex.\n"
   << "             You should never need to use this option.\n"
   << "   -h        This help\n"
   << "   -l        List devices\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -f file   Load a new application\n"
   << "   -v        Dump protocol data during operation\n"
   << "   -X        Reset device\n"
   << "   -z        Use non-threaded sockets\n"
   << "   -Z        Use threaded socket router (default)\n"
   << "\n"
   << endl;
}


bool ParseEpOverride(const char *arg, Usb::EndpointPair *epp)
{
	int read, write;
	char comma;
	istringstream iss(arg);
	iss >> hex >> read >> comma >> write;
	if( !iss )
		return false;
	epp->read = read;
	epp->write = write;
	return true;
}

class AutoClose
{
	FILE *fp;

public:
	AutoClose(FILE *fh) : fp(fh) {}
	~AutoClose()
	{
		fclose(fp);
	}
};

void SendAppFile(Barry::Mode::JavaLoader *javaloader, const char *filename)
{
	FILE *fp;

	char *data = NULL;

	codfile_header_t header;

	size_t n;
	size_t skip;
	off_t filesize;
	struct stat sb;


	// Get file size
	if (stat(filename, &sb) == -1) {
		throw runtime_error(string("Can't stat: ") + filename);
	}

	filesize = sb.st_size;
	if( filesize > (off_t)((size_t)-1) ) {
		throw runtime_error("Filesize larger than max fread()... contact Barry developers.");
	}

	// Open file
	fp = fopen(filename, "rb");

	if (fp == NULL) {
		throw runtime_error(string("Can't open: ") + filename);
	}

	AutoClose ac(fp);

	// Start
	javaloader->StartStream();

	// Read the file
	while (!feof(fp)) {
		n = fread(&header, sizeof(codfile_header_t), 1, fp);

		if (n != 1)
			continue;
	
		// Is a COD file packed (a big COD file) ?
		if (header.type == 0x4B50) {
			if (header.size1 != header.size2)
				continue;

			skip = header.strsize + header.strfree;

			if( fseek(fp, skip, SEEK_CUR) != 0 ) {
				throw runtime_error("Can't skip COD header");
			}

			// this is a one-time program, so allocate and
			// don't worry about freeing
			data = (char *) realloc(data, header.size1 * sizeof(char));

			n = fread(data, sizeof(char), header.size1, fp);
			if( n != header.size1 ) {
				throw runtime_error("Can't read packed COD header");
			}

			javaloader->SendStream(data, (int) header.size1);
		}
		// Is a simple COD file (a small COD file) ?
		else if (header.type == 0xC0DE) {
			rewind(fp);

			data = (char *) malloc(filesize * sizeof(char));

			n = fread(data, sizeof(char), filesize, fp);
			if( (off_t) n != filesize ) {
				throw runtime_error("Can't read COD data");
			}

			// Open stream
			javaloader->SendStream(data, filesize);
		}
	}

	// Stop
	javaloader->StopStream();
}


int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool load = false,
			list_only = false,
			data_dump = false,
			epp_override = false,
			threaded_sockets = true;
		string password;
		string filename;
		string busname;
		string devname;
		string iconvCharset;
		Usb::EndpointPair epOverride;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "B:e:f:hlN:p:P:R:vzZ");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'B':	// busname
				busname = optarg;
				break;

			case 'e':	// endpoint override
				if( !ParseEpOverride(optarg, &epOverride) ) {
					Usage();
					return 1;
				}
				epp_override = true;
				break;

			case 'N':	// Devname
				devname = optarg;
				break;

			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'P':	// Device password
				password = optarg;
				break;

			case 'f':	// Filename
				load = true;
				filename = optarg;
				break;

			case 'l':	// list only
				list_only = true;
				break;

			case 'v':	// data dump on
				data_dump = true;
				break;

			case 'z':	// non-threaded sockets
				threaded_sockets = false;
				break;

			case 'Z':	// threaded socket router
				threaded_sockets = true;
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
		Barry::Probe probe(busname.c_str(), devname.c_str());
		int activeDevice = -1;

		// show any errors during probe first
		if( probe.GetFailCount() ) {
			cout << "Blackberry device errors with errors during probe:" << endl;
			for( int i = 0; i < probe.GetFailCount(); i++ ) {
				cout << probe.GetFailMsg(i) << endl;
			}
		}


		// show all successfully found devices
		cout << "Blackberry devices found:" << endl;
		for( int i = 0; i < probe.GetCount(); i++ ) {
			if( data_dump )
				probe.Get(i).DumpAll(cout);
			else
				cout << probe.Get(i);
			cout << endl;
			if( probe.Get(i).m_pin == pin )
				activeDevice = i;
		}

		if( list_only )
			return 0;	// done

		if( activeDevice == -1 ) {
			if( pin == 0 ) {
				// can we default to single device?
				if( probe.GetCount() == 1 )
					activeDevice = 0;
				else {
					cerr << "No device selected" << endl;
					return 1;
				}
			}
			else {
				cerr << "PIN " << setbase(16) << pin
					<< " not found" << endl;
				return 1;
			}
		}

		cout << "Using device (PIN): " << setbase(16)
			<< probe.Get(activeDevice).m_pin << endl;

		// Override device endpoints if user asks
		Barry::ProbeResult device = probe.Get(activeDevice);
		if( epp_override ) {
			device.m_ep.read = epOverride.read;
			device.m_ep.write = epOverride.write;
			device.m_ep.type = 2;	// FIXME - override this too?
			cout << "Endpoint pair (read,write) overridden with: "
			     << hex
			     << (unsigned int) device.m_ep.read << ","
			     << (unsigned int) device.m_ep.write << endl;
		}


		//
		// Create our controller object
		//
		// Order is important in the following auto_ptr<> objects,
		// since Controller must get destroyed before router.
		// Normally you'd pick one method, and not bother
		// with auto_ptr<> and so the normal C++ constructor
		// rules would guarantee this safety for you, but
		// here we want the user to pick.
		//
		auto_ptr<SocketRoutingQueue> router;
		auto_ptr<Barry::Controller> pcon;
		if( threaded_sockets ) {
			router.reset( new SocketRoutingQueue );
			router->SpinoffSimpleReadThread();
			pcon.reset( new Barry::Controller(device, *router) );
		}
		else {
			pcon.reset( new Barry::Controller(device) );
		}

		Barry::Controller &con = *pcon;
		Barry::Mode::JavaLoader javaloader(con);

		//
		// execute each mode that was turned on
		//
		javaloader.Open(password.c_str());

		// Send the file
		if (load == true)
			SendAppFile(&javaloader, filename.c_str());
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

