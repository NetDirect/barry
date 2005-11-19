///
/// \file	btool.cc
///		Barry library tester
///
// FIXME - add copyright notices


#include "common.h"
#include "probe.h"
#include "usbwrap.h"
#include "error.h"
#include "controller.h"
#include <iostream>
#include <iomanip>
#include <getopt.h>

using namespace std;
using namespace Barry;

void Usage()
{
   cerr
   << "bbtool - Command line USB Blackberry Test Tool\n\n"
   << "   -h        This help\n"
   << "   -l        List devices\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device plugged in, this flag is optional\n"
   << endl;
}

int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool list_only = false;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "hlp:");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'l':	// list only
				list_only = true;
				break;

			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		Init();

		Probe probe;
		int activeDevice = -1;
		cout << "Blackberry devices found:" << endl;
		for( int i = 0; i < probe.GetCount(); i++ ) {
			cout << probe.Get(i) << endl;
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

		Controller con(probe.Get(activeDevice));
		con.Test();

	}
	catch( Barry::SBError &se ) {
		std::cerr << "SBError caught: " << se.what() << endl;
	}
	catch( Usb::UsbError &ue) {
		std::cerr << "UsbError caught: " << ue.what() << endl;
	}
	catch( std::runtime_error &re ) {
		std::cerr << "std::runtime_error caught: " << re.what() << endl;
		return 1;
	}

	return 0;
}

