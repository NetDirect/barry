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
   << "   -f file   Load a new application\n"
   << "   -h        This help\n"
   << "   -l        List COD files in device (use twice to list submodules)\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -v        Dump protocol data during operation\n"
   << "\n"
   << endl;
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
	if( (unsigned long)filesize > (size_t)-1 ) {
		throw runtime_error("Filesize larger than max fread()... contact Barry developers.");
	}

	// Open file
	fp = fopen(filename, "rb");

	if (fp == NULL) {
		throw runtime_error(string("Can't open: ") + filename);
	}

	AutoClose ac(fp);

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
}


int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool load = false,
			list_java = false,
			list_sub = false,
			data_dump = false;
		string password;
		string filename;
		string busname;
		string devname;
		string iconvCharset;
		Usb::EndpointPair epOverride;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "f:hlp:P:v");
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

			case 'f':	// Filename
				load = true;
				filename = optarg;
				break;

			case 'l':	// list device COD files
				if( !list_java )
					list_java = true;
				else
					list_sub = true;
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
		Barry::Mode::JavaLoader javaloader(con);

		//
		// execute each mode that was turned on
		//
		javaloader.Open(password.c_str());
		javaloader.StartStream();

		// Send the file
		if( load )
			SendAppFile(&javaloader, filename.c_str());

		if( list_java ) {
			JLDirectory dir;
			javaloader.GetDirectory(dir, list_sub);
			cout << dir;
		}

		// Stop
		javaloader.StopStream();

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

