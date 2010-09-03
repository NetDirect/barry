///
/// \file	usbclient.cc
///		Example code using the Barry library to interface
///		with a custom USB channel on a Blackberry device.
///

/*
    Copyright (C) 2010, RealVNC Ltd.

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
#include <cstring>
#include <string>

using namespace std;
using namespace Barry;

// This example is intended to provide similar functionality to the 'usbclient'
// example provided as a sample with BlackBerry JDEs. It is intended to
// be run with the 'usbdemo' sample application running on the BlackBerry. The
// 'usbdemo' sample application is provided as a sample with BlackBerry JDEs.
//
// A more complex use of Barry::RawChannel can be found in tools/brawchannel.cc

#define CHANNEL_NAME  "JDE_USBClient"

#define GCF_HEADER_SIZE 2

#define GCF_SIZE(buf) ((size_t)((buf[0] << 8) | (buf[1])))

#define HELLO_TO_BB   "Hello from PC"
#define HELLO_FROM_BB "Hello from Device"
#define BYE_TO_BB     "Goodbye from PC"
#define BYE_FROM_BB   "Goodbye from Device"

// Function to print the data read
static void printReply(Data &data)
{
	// Need to work out if it's a GCF wrapped reply or a USBPort based
	// reply.
	// To do this take the first two bytes as a length value and see
	// if they match the data length.
	string strdata;
	if(data.GetSize() < GCF_HEADER_SIZE ||
	    GCF_SIZE(data.GetData()) != (data.GetSize() - GCF_HEADER_SIZE) ) {
		// USBPort data
		strdata = string((const char*)data.GetData(), data.GetSize());
	}
	else {
		// GCF header present
		strdata = string((const char*)data.GetData() + GCF_HEADER_SIZE,
			      data.GetSize() - GCF_HEADER_SIZE);
	}
	cout << "Received data: " << strdata << endl;
	
}

int main(int argc, char *argv[])
{
	try {
		Barry::Init();

		Barry::Probe probe;
		if( probe.GetCount() == 0 ) {
			cout << "No Blackberry found!" << endl;
			return 1;
		}
		else {
			cout << "Using PIN: "
			     << probe.Get(0).m_pin.str() << endl;
		}
		
		// Create the router
		auto_ptr<SocketRoutingQueue> router;
		router.reset(new SocketRoutingQueue());
		router->SpinoffSimpleReadThread();

		// Create our controller object
		Barry::Controller con(probe.Get(0), *router);

		// Create the raw channel
		Barry::Mode::RawChannel rawChannel(con);

		cout << "Connecting to device channel with an empty password" << endl;
		rawChannel.Open("", CHANNEL_NAME);

		cout << "Sending hello to device: " << HELLO_TO_BB << endl;
		Data data(HELLO_TO_BB, sizeof(HELLO_TO_BB) - 1);
		rawChannel.Send(data);

		cout << "Waiting for reply from device" << endl;
		rawChannel.Receive(data);
		printReply(data);

		cout << "Sending bye bye to the device: " << BYE_TO_BB << endl;
		data = Data(BYE_TO_BB, sizeof(BYE_TO_BB) - 1);
		rawChannel.Send(data);

		cout << "Waiting for response from device" << endl;
		rawChannel.Receive(data);
		printReply(data);

		cout << "All send and received OK, exiting" << endl;

	}
	catch( Barry::Error &e ) {
		cerr << "Exception caught: " << e.what() << endl;
		return 1;
	}
	catch( std::exception &e ) {
		cerr << "Exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

