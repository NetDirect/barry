///
/// \file	addcalendar.cc
///		Example code using the Barry library to add a calendar
///		entry to a Blackberry device.
///

/*
    Copyright (C) 2006-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#define _XOPEN_SOURCE
#include <time.h>
#include <string.h>
#include <barry/barry.h>
#include <iostream>

using namespace std;
using namespace Barry;


void ReadLine(const char *prompt, std::string &data)
{
	cout << prompt << ": ";
	getline(cin, data);
}

bool ReadBool(const char *prompt)
{
	cout << prompt << "? (y/n) ";
	string data;
	getline(cin, data);
	return data[0] == 'y' || data[0] == 'Y';
}

time_t ReadTime(const char *prompt)
{
	string data;
	char *unprocessed = 0;
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_isdst = -1;

	do {
		cout << prompt << ": ";
		getline(cin, data);
		unprocessed = strptime(data.c_str(), "%Y/%m/%d %H:%M", &tm);
	} while( !unprocessed );

	return mktime(&tm);
}

void ReadInput(Barry::Calendar &cal)
{
	cal.AllDayEvent = ReadBool("All Day Event");

	cout << "Note: enter dates in the following format: YYYY/MM/DD hh:mm\n";
	cout << "Time is in 24 hour format\n\n";
	cal.StartTime = ReadTime("Start Time");
	cal.EndTime = ReadTime("End Time");
	cal.NotificationTime = ReadTime("Notification Time");

	ReadLine("Subject", cal.Subject);
	ReadLine("Notes", cal.Notes);
	ReadLine("Location", cal.Location);

	cal.Recurring = false;
}

void Upload(const Barry::ProbeResult &device, const Barry::Calendar &cal)
{
	// connect to address book
	Controller con(device);
	Mode::Desktop desktop(con);
	desktop.Open();
	unsigned int id = desktop.GetDBID("Calendar");

	// find out what records are already there, and make new record ID
	RecordStateTable table;
	desktop.GetRecordStateTable(id, table);
	uint32_t recordId = table.MakeNewRecordId();

	// add it
	desktop.AddRecordByType(recordId, cal);
	cout << "Added successfully: " << endl << cal << endl;
}

int main(int argc, char *argv[])
{
	try {

		Barry::Init(true);

		Barry::Probe probe;
		if( probe.GetCount() == 0 ) {
			cout << "No Blackberry found!" << endl;
			return 1;
		}


		Barry::Calendar cal;
		ReadInput(cal);
		cout << "Just before upload: " << cal << endl;
		Upload(probe.Get(0), cal);

	}
	catch( std::exception &e ) {
		std::cerr << "Exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

