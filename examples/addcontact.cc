///
/// \file	addcontact.cc
///		Example code using the Barry library to add a contact
///		to a Blackberry device.
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

#include <barry/barry.h>
#include <iostream>

using namespace std;
using namespace Barry;


void ReadLine(const char *prompt, std::string &data)
{
	cout << prompt << ": ";
	getline(cin, data);
}

void ReadInput(Barry::Contact &contact)
{
	ReadLine("First Name", contact.FirstName);
	ReadLine("Last Name", contact.LastName);
	ReadLine("Job Title", contact.JobTitle);
	ReadLine("Email Address", contact.Email);
	ReadLine("Main Phone Number", contact.Phone);
	ReadLine("Home Phone Number", contact.HomePhone);
	ReadLine("Work Phone Number", contact.WorkPhone);
	ReadLine("Fax Number", contact.Fax);
	ReadLine("Cell Number", contact.MobilePhone);
	ReadLine("Pager Number", contact.Pager);
	ReadLine("Company", contact.Company);
	ReadLine("Address Line 1", contact.WorkAddress.Address1);
	ReadLine("Address Line 2", contact.WorkAddress.Address2);
	ReadLine("Address Line 3", contact.WorkAddress.Address3);
	ReadLine("City", contact.WorkAddress.City);
	ReadLine("Province / State", contact.WorkAddress.Province);
	ReadLine("Country", contact.WorkAddress.Country);
	ReadLine("Postal / Zip Code", contact.WorkAddress.PostalCode);
	ReadLine("Notes", contact.Notes);

	string categories;
	ReadLine("Categories", categories);
	Contact::CategoryStr2List(categories, contact.Categories);
}

void Upload(const Barry::ProbeResult &device, const Barry::Contact &contact)
{
	// connect to address book
	Controller con(device);
	Mode::Desktop desktop(con);
	desktop.Open();
	unsigned int id = desktop.GetDBID("Address Book");

	// find out what records are already there, and make new record ID
	RecordStateTable table;
	desktop.GetRecordStateTable(id, table);
	uint32_t recordId = table.MakeNewRecordId();

	// add it
	desktop.AddRecordByType(recordId, contact);
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


		Barry::Contact contact;
		ReadInput(contact);
		Upload(probe.Get(0), contact);

	}
	catch( std::exception &e ) {
		std::cerr << "Exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

