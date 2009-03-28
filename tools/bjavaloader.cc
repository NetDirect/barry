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
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <getopt.h>
#include <fstream>
#include <string.h>

// supported javaloader commands
#define CMD_LIST	"dir"
#define CMD_ERASE	"erase"
#define CMD_LOAD	"load"
#define CMD_SCREENSHOT	"screenshot"
#define CMD_SETTIME	"settime"
#define CMD_EVENTLOG	"eventlog"
#define CMD_CLEAR_LOG	"cleareventlog"
#define CMD_SAVE	"save"
#define CMD_DEVICEINFO	"deviceinfo"
#define CMD_WIPE	"wipe"
#define CMD_LOGSTRACES	"logstacktraces"

// time string format specifier and user friendly description
#define TIME_FMT         "%Y-%m-%d %H:%M:%S"
#define TIME_FMT_EXAMPLE "yyyy-mm-dd HH:MM:SS"

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
   << "   -a        Wipe applications only\n"
   << "   -i        Wipe filesystem only\n"
   << "   -f        Force erase, if module is in use\n"
   << "   -h        This help\n"
   << "   -s        List sibling in module list\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -v        Dump protocol data during operation\n"
   << "\n"
   << "commands\n"
   << "\n"
   << "   " << CMD_LIST << " [-s]\n"
   << "      Lists modules on the handheld\n"
   << "\n"
   << "   " << CMD_DEVICEINFO << "\n"
   << "      Provides information on the handheld\n"
   << "\n"
   << "   " << CMD_LOAD << " <.cod file> ...\n"
   << "      Loads modules onto the handheld\n"
   << "\n"
   << "   " << CMD_SAVE << " <module name> ...\n"
   << "      Retrieves modules from the handheld and writes to .cod file\n"
   << "      Note: will overwrite existing files!\n"
   << "\n"
   << "   " << CMD_WIPE << " [-a | -i]\n"
   << "      Wipes the handheld\n"
   << "      Use Caution: Wiping filesystem will remove all data\n"
   << "                   such as messages, contacts, etc.\n"
   << "                   Wiping applications will remove all .cod files\n"
   << "                   on the device, including OS .cod files.\n"
   << "\n"
   << "   " << CMD_ERASE << " [-f] <module name> ...\n"
   << "      Erase module from handheld\n"
   << "\n"
   << "   " << CMD_EVENTLOG << "\n"
   << "      Retrieves the handheld event log\n"
   << "\n"
   << "   " << CMD_CLEAR_LOG << "\n"
   << "      Clears the handheld event log\n"
   << "\n"
   << "   " << CMD_LOGSTRACES << "\n"
   << "      Dump the stack traces for all threads to the event log\n"
   << "\n"
   << "   " << CMD_SCREENSHOT << " <.bmp file>\n"
   << "      Make a screenshot of handheld\n"
   << "\n"
   << "   " << CMD_SETTIME << " [" << TIME_FMT_EXAMPLE << "]\n"
   << "      Sets the time on the handheld to the current time\n"
   << "      Or the time specified as an argument to " << CMD_SETTIME << "\n"
   << "      If given as argument, current system timezone is assumed\n"
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

void SetTime(Barry::Mode::JavaLoader *javaloader, const char *timestr)
{
	time_t when;

	if( timestr ) {
		struct tm timeinfo;
		memset(&timeinfo, 0, sizeof(timeinfo));

		// parse time string
		char *p = strptime(timestr, TIME_FMT, &timeinfo);

		// NULL is return when strptime fails to match all of the format
		// string, and returns a pointer to the NULL byte at the end of
		// the input string on success
		if( p == NULL || p != (timestr + strlen(timestr)) ) {
			throw runtime_error(string("Unable to parse time string: ") + timestr);
		}

		when = mktime(&timeinfo);
	} else { // time string is NULL, get current time
		time(&when);
	}

	javaloader->SetTime(when);
}

void SendAppFile(Barry::Mode::JavaLoader *javaloader, const char *filename)
{
	ifstream file(filename);
	javaloader->LoadApp(file);
}

void GetScreenshot(Barry::Mode::JavaLoader *javaloader, const char *filename)
{

	// Take a screenshot
	//   - info object contains the screenshot properties (width, height...)
	//   - image will be filled with the raw pixel screenshot data
	JLScreenInfo info;
	Data image;
	javaloader->GetScreenshot(info, image);


	// Convert to BMP format
	Data bitmap(-1, GetTotalBitmapSize(info));
	ScreenshotToBitmap(info, image, bitmap);

	// Write BMP file
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL) {
		throw runtime_error(string("Can't open: ") + filename);
	}
	AutoClose ac(fp);

	fwrite(bitmap.GetData(), bitmap.GetSize(), 1, fp);
}

void SaveModule(Barry::Mode::JavaLoader *javaloader, const char *filename)
{
	string fname(filename), module;

	size_t ext_index = fname.rfind(".cod");
	if( ext_index != string::npos ) {
		// filename contains .cod extension, strip it for module name
		module = fname.substr(0, ext_index);
	}
	else {
		// filename does not contain .cod extension, use it as module name
		module = fname;
		// append extension to file name
		fname.append(".cod");
	}

	ofstream file(fname.c_str(), ios::binary | ios::trunc);
	javaloader->Save(module.c_str(), file);
}


int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool list_siblings = false,
			force_erase = false,
			data_dump = false,
			wipe_apps = true,
			wipe_fs = true;
		string password;
		vector<string> params;
		string busname;
		string devname;
		string iconvCharset;
		Usb::EndpointPair epOverride;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "aifhsp:P:v");
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

			case 'f':	// turn on 'force' mode for erase
				force_erase = true;
				break;

			case 's':	// turn on listing of sibling modules
				list_siblings = true;
				break;

			case 'v':	// data dump on
				data_dump = true;
				break;

			case 'a':	// wipe apps only
				wipe_fs = false;
				break;

			case 'i':	// wipe filesystem
				wipe_apps = false;
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

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice));
		Barry::Mode::JavaLoader javaloader(con);

		//
		// execute each mode that was turned on
		//
		javaloader.Open(password.c_str());
		javaloader.StartStream();

		if( cmd == CMD_LIST ) {
			JLDirectory dir;
			javaloader.GetDirectory(dir, list_siblings);
			cout << dir;
		}
		else if( cmd == CMD_LOAD ) {
			if( params.size() == 0 ) {
				cerr << "specify at least one .cod file to load" << endl;
				Usage();
				return 1;
			}

			vector<string>::iterator i = params.begin(), end = params.end();
			for( ; i != end; ++i ) {
				cout << "loading " << (*i) << "... ";
				SendAppFile(&javaloader, (*i).c_str());
				cout << "done." << endl;
			}
		}
		else if( cmd == CMD_ERASE ) {
			if( params.size() == 0 ) {
				cerr << "specify at least one module to erase" << endl;
				Usage();
				return 1;
			}

			vector<string>::iterator i = params.begin(), end = params.end();
			for( ; i != end; ++i ) {
				cout << "erasing: " << (*i) << "... ";
				if( force_erase )
					javaloader.ForceErase((*i));
				else
					javaloader.Erase((*i));
				cout << "done." << endl;
			}
		}
		else if( cmd == CMD_SCREENSHOT ) {
			if( params.size() == 0 ) {
				cerr << "specify a .bmp filename" << endl;
				Usage();
				return 1;
			}

			GetScreenshot(&javaloader, params[0].c_str());
		}
		else if( cmd == CMD_SETTIME ) {
			if( params.size() > 0 ) {
				SetTime(&javaloader, params[0].c_str());
			} else {
				SetTime(&javaloader, NULL);
			}
		}
		else if( cmd == CMD_EVENTLOG ) {
			JLEventlog log;
			javaloader.GetEventlog(log);
			cout << log;
		}
		else if( cmd == CMD_CLEAR_LOG ) {
			javaloader.ClearEventlog();
		}
		else if( cmd == CMD_LOGSTRACES ) {
			javaloader.LogStackTraces();
		}
		else if( cmd == CMD_SAVE ) {
			if( params.size() == 0 ) {
				cerr << "specify at least one module to save" << endl;
				Usage();
				return 1;
			}

			vector<string>::iterator i = params.begin(), end = params.end();
			for( ; i != end; ++i ) {
				cout << "saving: " << (*i) << "... ";
				SaveModule(&javaloader, (*i).c_str());
				cout << "done." << endl;
			}
		}
		else if( cmd == CMD_DEVICEINFO ) {
			JLDeviceInfo info;
			javaloader.DeviceInfo(info);
			cout << info;
		}
		else if( cmd == CMD_WIPE ) {
			cout
				<< "Use Caution: Wiping filesystem will remove all data\n"
				<< "             such as messages, contacts, etc.\n"
				<< "             Wiping applications will remove all .cod files\n"
				<< "             on the device, including OS .cod files.\n\n"
				<< "Continue with wipe? (yes/no) ";
			string confirm;
			getline(cin, confirm);
			if( confirm == "yes" ) {
				javaloader.Wipe(wipe_apps, wipe_fs);
			}
			else {
				cout << "Response of 'yes' not received, aborting." << endl;
			}
		}
		else {
			cerr << "invalid command \"" << cmd << "\"" << endl;
			Usage();
			return 1;
		}

		// Stop
		javaloader.StopStream();

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

