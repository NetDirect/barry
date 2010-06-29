///
/// \file	bvncrelay.cc
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
#include <algorithm>
#include <getopt.h>
#include <fstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "i18n.h"

using namespace std;
using namespace Barry;

class TCPWriter : public Barry::Mode::VNCServerDataCallback
{
public:
    TCPWriter(int socket, bool& keepGoing)
    {
        cfd = socket;
        continuePtr = &keepGoing;
    }
    
    void DataReceived(Data& data)
    {
        std::cerr << "From BB: ";
        data.DumpHex(std::cerr);
        std::cerr << "\n";

        size_t toWrite = data.GetSize();
        size_t written = 0;

        while (written < toWrite)
        {
            ssize_t writtenThisTime = write(cfd, &(data.GetData()[written]), toWrite - written);
            cerr << "Written " << writtenThisTime << " bytes over TCP\n";
            if (writtenThisTime < 0)
            {
                *continuePtr = false;
            }
            else
            {
                written += writtenThisTime;
            }
        }
    }

private:
    bool* continuePtr;
    int cfd;
};

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "bvncrelay - Command line USB Blackberry VNC Relay\n"
   << "        Copyright 2010, RealVNC Ltd.\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -h        This help\n"
   << "   -s        List sibling in module list\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -v        Dump protocol data during operation\n"
   << "\n"
   << endl;
}

static void handle_error(const char* text)
{
    throw new std::runtime_error(text);
}


int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool list_siblings = false,
			data_dump = false;
		string password;
		vector<string> params;
		string busname;
		string devname;
		string iconvCharset;
		Usb::EndpointPair epOverride;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "hsp:P:v");
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

			case 's':	// turn on listing of sibling modules
				list_siblings = true;
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
			cerr << "missing command" << endl;
			Usage();
			return 1;
		}

		// Fetch command from remaining arguments
		string cmd = argv[0];
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

        // Now start to read from TCP and get ready to write
        // to the BlackBerry.
        int sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
            handle_error("socket");

        cerr << "Trying to listen...\n";

        sockaddr_in my_addr;
        memset(&my_addr, 0, sizeof(struct sockaddr_in));
                               /* Clear structure */
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(2003);
        my_addr.sin_addr.s_addr = INADDR_ANY;
    
        if (bind(sfd, (struct sockaddr *) &my_addr,
                sizeof(struct sockaddr_in)) == -1)
            handle_error("bind");

        std::cerr << "Bound\n";
        if (listen(sfd, 1) == -1)
            handle_error("listen");
        std::cerr << "Listening\n";

        /* Now we can accept incoming connections one
           at a time using accept(2) */

        sockaddr_in peer_addr;
        socklen_t peer_addr_size = sizeof(struct sockaddr_in);
        int cfd = accept(sfd, (struct sockaddr *) &peer_addr,
                     &peer_addr_size);
        if (cfd == -1)
            handle_error("accept");

        std::cerr << "Accepted\n";

        bool running = true;

        // Create the thing which will write onto TCP
        TCPWriter tcpwriter(cfd, running);

        // Set up the BlackBerry gubbins
        // Start a thread to handle any data arriving from
        // the BlackBerry.
        auto_ptr<SocketRoutingQueue> router;
        router.reset(new SocketRoutingQueue);
        router->SpinoffSimpleReadThread();

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice), *router);
		Barry::Mode::VNCServer vncrelay(con, tcpwriter);

		//
		// execute each mode that was turned on
		//
		vncrelay.Open(password.c_str());

        // We now have a thread running to read from the
        // BB and write over TCP; in this thread we'll
        // read from TCP and write to the BB.
        unsigned char buf[2000];
        while (running)
        {
            size_t haveRead = read(cfd, buf, 2000);
            if (haveRead > 0)
            {
                std::cerr << "Sending " << haveRead << " bytes TCP->USB\n";
                Data toWrite(buf, haveRead);
                std::cerr << "To BB: ";
                toWrite.DumpHex(std::cerr);
                std::cerr << "\n";
                vncrelay.Send(toWrite);
                std::cerr << "Sent " << haveRead << " bytes TCP->USB\n";
            }
            else if (haveRead < 0)
            {
                running = false;
            }
        }
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

