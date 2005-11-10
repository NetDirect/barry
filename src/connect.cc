///
/// \file	connect.cc
///		Connection tester.
///
// FIXME - add copyright notices


#include "usbwrap.h"
#include "data.h"

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <pthread.h>

using namespace std;
using namespace Usb;

#define DATA_PATH		"data"
#define WAIT_TIMEOUT		5		// currently in seconds

bool bTestMode = false;


IO WaitOnRead(Device &dev, int read_ep)
{
	cout << "WaitOnRead" << endl;

	if( bTestMode )
		return IO::InvalidIO();

	// wait for responses
	time_t start = time(NULL);
	unsigned long nLoopCount = 0;
	while( time(NULL) - start < WAIT_TIMEOUT ) {
		nLoopCount++;
		usleep(100);
//		pthread_yield();
		IO io = dev.PollCompletions();
		if( io.IsValid() ) {
			if( io.GetEndpoint() == read_ep ) {
				return io;
			}
			else {
				// not the completion we were expecting, free
				cout << "completion found: "
					<< io.GetStatus()
					<< " -- "
					<< "endpoint: " <<
					io.GetEndpoint() << endl;
			}
		}
	}

	// if we get here, we've timed out, quit
//	dev.ClearIO();
	cout << "WaitOnRead loop count: " << nLoopCount << endl;
	throw std::runtime_error("Timeout in WaitOnRead");
}

void Process(const Data &expected, Data &readbuf, IO io)
{
	cout << "Process" << endl;
	if( bTestMode ) {
		readbuf.ReleaseBuffer();
		cout << Diff(expected, readbuf) << endl;
		return;
	}

	if( !io.IsValid() )
		return;		// nothing to do

	// io is an io handle of the expected read, so get the data
	cout << "read complete: "
		<< io.GetStatus() << endl;
	readbuf.ReleaseBuffer(io.GetSize());
	cout << "endpoint: " << io.GetEndpoint() << endl;
	cout << "diff of expected/readbuf:" << endl;
	cout << Diff(expected, readbuf) << endl;
}

void ClearIO(Device &dev)
{
	cout << "ClearIO" << endl;
	// wait for all pending IO to finish, and throw away (for now)
	time_t start = time(NULL);
	unsigned long nLoopCount = 0;
	while( dev.GetIOCount() && (time(NULL) - start) < WAIT_TIMEOUT ) {
		nLoopCount++;
		usleep(100);
//		pthread_yield();
		IO io = dev.PollCompletions();
	}

	if( time(NULL) - start >= WAIT_TIMEOUT ) {
		cout << "ClearIO loop count: " << nLoopCount << endl;
		throw std::runtime_error("Timeout in ClearIO");
	}
}

void sequence(Device &dev, int write_ep, int read_ep, const string &param)
{
	typedef vector<Data> DataVec;
	DataVec sequence;
	if( !LoadDataArray(param, sequence) ) {
		cout << "Unable to load data sequence from: " << param << endl;
		return;
	}

	// keep a space for reading
	Data readbuf(-1, 0x4000);

	DataVec::iterator b = sequence.begin(), e = sequence.end();
	bool bReadPending = false;
	int WriteCount = 0;
	for( ; b != e; b++ ) {

//		try {

		if( b->GetEndpoint() == write_ep ) {
			// according to the captures, every write of a
			// command sets up a read first, to get the
			// response... so do that now
			cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";

			ClearIO(dev);

			cout << "SetupRead(" << read_ep << ")" << endl;
			dev.TrackBulkRead(read_ep, readbuf);

			cout << "SetupWrite()" << endl;
			cout << "Sending data to endpoint: " << write_ep
			     << " (" << ++WriteCount << ")" << endl;
			cout << *b;
			dev.TrackBulkWrite(write_ep, *b);

			bReadPending = true;
		}
		else {
			// this is a read... if we have a read pending,
			// wait for data from the device and then compare it
			// otherwise, setup the read and then wait
			cout << "Read Cycle" << endl;

			if( !bReadPending ) {
				// no read pending, and we have a read buffer
				// setup read before waiting
				cout << "Read Cycle: SetupRead("
				     << read_ep << ")" << endl;
//pthread_yield();
//usleep(500000);
				dev.TrackBulkRead(read_ep, readbuf);
			}
			else
				bReadPending = false;

			// wait and process data read
			Process(*b, readbuf, WaitOnRead(dev, read_ep));
		}

//		} catch( std::runtime_error &re ) {
//			cout << "Caught: " << re.what() << endl;
//		}
	}
}

void commandline(Device &dev)
{
	while( cin ) {
		string command, param;
		cout << "> ";
		cin >> command;
		if( command == "s" ) {
			int wr_ep, rd_ep;
			cin >> setbase(16) >> wr_ep >> rd_ep >> param;
			cout << endl;
			sequence(dev, wr_ep, rd_ep, param);
		}
		else if( command == "help" || command == "h" ) {
			cout << endl
			<< "s [write_ep] [read_ep] [file] - data sequence\n"
			<< "help, h - help\n"
			<< "q - quit\n"
			<< endl;
		}
		else if( command == "q" ) {
			return;
		}
		else {
			cout << "\nunknown command: " << command << endl;
		}

		if( cin.fail() && !cin.eof() ) {
			cin.clear();
			cin.ignore(1024, '\n');
		}
	}
}


int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		libusb_set_debug(9);
		libusb_init();

		if( argc >= 2 && string(argv[1]) == "-t" ) {
			// special test mode, without a device
			bTestMode = true;
			Device dev(1);
			commandline(dev);
			return 0;
		}

		Match match(VENDOR_RIM, PRODUCT_RIM_BLACKBERRY);

		libusb_device_id_t devid;
		if (match.next_device(&devid)) {
			cout << "found device id " << devid << endl;
			Device dev(devid);
			dev.Reset();
			sleep(5);

			Interface iface(dev, 0);

			if( !dev.SetConfiguration(1) )
				throw std::runtime_error("SetConfiguration failed");

			commandline(dev);
		}
		else {
			cout << "No Blackberry devices found!" << endl;
		}

	}
	catch( std::runtime_error &re ) {
		std::cerr << re.what() << endl;
		return 1;
	}

	return 0;
}

