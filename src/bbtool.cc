///
/// \file	bbtool.cc
///		Syncberry library tester
///
// FIXME - add copyright notices


#include "sbcommon.h"
#include "probe.h"
#include "usbwrap.h"
#include "error.h"
#include <iostream>

using namespace std;
using namespace Syncberry;

int main()
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		Init();

		Probe probe;
		cout << "Blackberry devices found:" << endl;
		for( int i = 0; i < probe.GetCount(); i++ ) {
			cout << probe.Get(i) << endl;
		}

	}
	catch( Syncberry::SBError &se ) {
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

