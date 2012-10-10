///
/// \file	brawchannel.cc
///		Directs a named raw channel over STDIN/STDOUT or TCP
///

/*
    Copyright (C) 2010-2012, RealVNC Ltd.

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
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "i18n.h"
#include "platform.h"
#include "barrygetopt.h"
#include "brawchannel.h"

using namespace std;
using namespace Barry;

static volatile bool signalReceived = false;

static void signalHandler(int signum)
{
	signalReceived = true;
}

class CallbackHandler : public Barry::Mode::RawChannelDataCallback
{
private:
	OutputStream& m_output;
	volatile bool *m_continuePtr;
	bool m_verbose;

public:
	CallbackHandler(OutputStream& output, volatile bool &keepGoing, bool verbose)
		: m_output(output)
		, m_continuePtr(&keepGoing)
		, m_verbose(verbose)
		{
		}


public: // From RawChannelDataCallback
	virtual void DataReceived(Data &data);
	virtual void ChannelError(string msg);
	virtual void ChannelClose();
};


void CallbackHandler::DataReceived(Data &data)
{
	if( m_verbose ) {
		cerr << _("From BB: ");
		data.DumpHex(cerr);
		cerr << "\n";
	}

	size_t toWrite = data.GetSize();
	size_t written = 0;

	while( written < toWrite && *m_continuePtr ) {
		ssize_t writtenThisTime = m_output.write(&(data.GetData()[written]), toWrite - written);
		if( m_verbose ) {
			cerr.setf(ios::dec, ios::basefield);
			cerr << string_vprintf(_("Written %ld bytes over stdout"), (long int)writtenThisTime) << endl;
		}
		fflush(stdout);
		if( writtenThisTime < 0 ) {
			ChannelClose();
		}
		else {
			written += writtenThisTime;
		}
	}
}

void CallbackHandler::ChannelError(string msg)
{
	cerr << _("CallbackHandler: Received error: ") << msg << endl;
	ChannelClose();
}

void CallbackHandler::ChannelClose()
{
	*m_continuePtr = false;
}

void Usage()
{
	int logical, major, minor;
	const char *Version = Barry::Version(logical, major, minor);

	cerr << string_vprintf(
	_("brawchannel - Command line USB Blackberry raw channel interface\n"
	"        Copyright 2010-2012, RealVNC Ltd.\n"
	"        Using: %s\n"
	"\n"
	"Usage:\n"
	"brawchannel [options] <channel name>\n"
	"\n"
	"   -h        This help\n"
	"   -p pin    PIN of device to talk with\n"
	"             If only one device is plugged in, this flag is optional\n"
	"   -P pass   Simplistic method to specify device password\n"
	"   -l port   Listen for a TCP connection on the provided port instead\n"
	"             of using STDIN and STDOUT for data\n"
	"   -a addr   Address to bind the listening socket to, allowing listening\n"
	"             only on a specified interface\n"
	"   -v        Dump protocol data during operation\n"
	"             This will cause libusb output to appear on STDOUT unless\n"
	"             the environment variable USB_DEBUG is set to 0,1 or 2.\n"),
		Version)
	<< endl;
}

// Helper class to restore signal handlers when shutdown is occuring
// This class isn't responsible for setting up the signal handlers
// as they need to be restored before the Barry::Socket starts closing.
class SignalRestorer
{
private:
	int m_signum;
	sighandler_t m_handler;
public:
	SignalRestorer(int signum, sighandler_t handler)
		: m_signum(signum), m_handler(handler) {}
	~SignalRestorer() { signal(m_signum, m_handler); }
};

int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	// Setup signal handling
	sighandler_t oldSigHup = signal(SIGHUP, &signalHandler);
	sighandler_t oldSigTerm = signal(SIGTERM, &signalHandler);
	sighandler_t oldSigInt = signal(SIGINT, &signalHandler);
	sighandler_t oldSigQuit = signal(SIGQUIT, &signalHandler);

	cerr.sync_with_stdio(true);	// since libusb uses
					// stdio for debug messages

	// Buffer to hold data read in from STDIN before sending it
	// to the BlackBerry.
	unsigned char *buf = NULL;
	try {
		uint32_t pin = 0;
		bool data_dump = false;
		string password;
		string tcp_addr;
		long tcp_port = 0;

		// process command line options
		for( ;; ) {
			int cmd = getopt(argc, argv, "hp:P:l:a:v");
			if( cmd == -1 ) {
				break;
			}

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

			case 'l':
				tcp_port = strtol(optarg, NULL, 10);
				break;

			case 'a':
				tcp_addr = optarg;
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
			cerr << _("Error: Missing raw channel name.") << endl;
			Usage();
			return 1;
		}

		if( argc > 1 ) {
			cerr << _("Error: Too many arguments.") << endl;
			Usage();
			return 1;
		}

		// Fetch command from remaining arguments
		string channelName = argv[0];
		argc --;
		argv ++;

		if( tcp_addr.length() != 0 && tcp_port == 0 ) {
			cerr << _("Error: specified TCP listen address but no port.") << endl;
			return 1;
		}

		if( data_dump ) {
			// Warn if USB_DEBUG isn't set to 0, 1 or 2
			// as that usually means libusb will write to STDOUT
			char *val = getenv("USB_DEBUG");
			int parsedValue = -1;
			if( val ) {
				parsedValue = atoi(val);
			}
			if( parsedValue != 0 && parsedValue != 1 && parsedValue != 2 ) {
				cerr << _("Warning: Protocol dump enabled without setting USB_DEBUG to 0, 1 or 2.\n"
				     "         libusb might log to STDOUT and ruin data stream.") << endl;
			}
		}

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump, &cerr);

		// Probe the USB bus for Blackberry devices.
		// If user has specified a PIN, search for it in the
		// available device list here as well
		Barry::Probe probe;
		int activeDevice = probe.FindActive(pin);
		if( activeDevice == -1 ) {
			cerr << _("No device selected, or PIN not found")
				<< endl;
			return 1;
		}

		// Now get setup to open the channel.
		if( data_dump ) {
			cerr << _("Connected to device, starting read/write\n");
		}

		volatile bool running = true;

		auto_ptr<TcpStream> tcpStreamPtr;
		auto_ptr<InputStream> inputPtr;
		auto_ptr<OutputStream> outputPtr;
		
		if( tcp_port != 0 ) {
			/* Use TCP socket for channel data */
			tcpStreamPtr.reset(new TcpStream(tcp_addr, tcp_port));
			if( !tcpStreamPtr->accept() ) {
				cerr << _("Failed to listen on requested port\n");
				return 1;
			}
			inputPtr.reset(new TcpInStream(*tcpStreamPtr));
			outputPtr.reset(new TcpOutStream(*tcpStreamPtr));
		} else {
			/* Use STDIN and STDOUT for channel data */
			inputPtr.reset(new StdInStream());
			outputPtr.reset(new StdOutStream());
		}
		// Create the thing which will write onto stdout
		// and perform other callback duties.
		CallbackHandler callbackHandler(*outputPtr, running, data_dump);

		// Start a thread to handle any data arriving from
		// the BlackBerry.
		auto_ptr<SocketRoutingQueue> router;
		router.reset(new SocketRoutingQueue());
		router->SpinoffSimpleReadThread();

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice), *router);

		Barry::Mode::RawChannel rawChannel(con, callbackHandler);

		// Try to open the requested channel now everything is setup
		rawChannel.Open(password.c_str(), channelName.c_str());

		// We now have a thread running to read from the
		// BB and write over stdout; in this thread we'll
		// read from stdin and write to the BB.
		const size_t bufSize = rawChannel.MaximumSendSize();
		buf = new unsigned char[bufSize];

		// Set up the signal restorers to restore signal
		// handling (in their destructors) before the socket
		// starts to be closed. This allows, for example,
		// double control-c presses to stop graceful close
		// down.
		SignalRestorer srh(SIGHUP, oldSigHup);
		SignalRestorer srt(SIGTERM, oldSigTerm);
		SignalRestorer sri(SIGINT, oldSigInt);
		SignalRestorer srq(SIGQUIT, oldSigQuit);

		while( running && !signalReceived ) {
			ssize_t haveRead = inputPtr->read(buf, bufSize, READ_TIMEOUT_SECONDS);
			if( haveRead > 0 ) {
				Data toWrite(buf, haveRead);
				if( data_dump ) {
					cerr.setf(ios::dec, ios::basefield);
					cerr << string_vprintf(_("Sending %ld bytes stdin->USB\n"), (long int)haveRead);
					cerr << _("To BB: ");
					toWrite.DumpHex(cerr);
					cerr << "\n";
				}
				rawChannel.Send(toWrite);
				if( data_dump ) {
					cerr.setf(ios::dec, ios::basefield);
					cerr << string_vprintf(_("Sent %ld bytes stdin->USB\n"), (long int)haveRead);
				}
			}
			else if( haveRead < 0 ) {
				running = false;
			}
		}
	}
	catch( const Usb::Error &ue ) {
		cerr << _("Usb::Error caught: ") << ue.what() << endl;
		return 1;
	}
	catch( const Barry::Error &se ) {
		cerr << _("Barry::Error caught: ") << se.what() << endl;
		return 1;
	}
	catch( const exception &e ) {
		cerr << _("exception caught: ") << e.what() << endl;
		return 1;
	}
	catch( ... ) {
		cerr << _("unknown exception caught") << endl;
		return 1;
	}

	delete[] buf;

	return 0;
}

