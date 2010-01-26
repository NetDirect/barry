///
/// \file	addmemo.cc
///		Example code using the Barry library to add a memo
///		to a Blackberry device.
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <fstream>
#include <iomanip>

using namespace std;
using namespace Barry;


void ReadLine(const char *prompt, std::string &data)
{
	cout << prompt << ": ";
	getline(cin, data);
}

void ReadInput(Barry::Memo &memo)
{
	ReadLine("Title", memo.Title);

	string body;
	do {
		ReadLine("Body line (blank to end)", body);
		if( body.size() ) {
			if( memo.Body.size() )
				memo.Body += "\n";
			memo.Body += body;
		}
	} while( body.size() );

	string categories;
	ReadLine("Categories", categories);
	memo.Categories.CategoryStr2List(categories);
}

void Upload(const Barry::ProbeResult &device, const Barry::Memo &memo)
{
	// connect to address book
	Controller con(device);
	Mode::Desktop desktop(con);
	desktop.Open();
	unsigned int id = desktop.GetDBID(Barry::Memo::GetDBName());

	// find out what records are already there, and make new record ID
	RecordStateTable table;
	desktop.GetRecordStateTable(id, table);
	uint32_t recordId = table.MakeNewRecordId();

	// add it
	desktop.AddRecordByType(recordId, memo);
	cout << "Added successfully." << endl;
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


		Barry::Memo memo;
		ReadInput(memo);
		Upload(probe.Get(0), memo);

	}
	catch( std::exception &e ) {
		std::cerr << "Exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

