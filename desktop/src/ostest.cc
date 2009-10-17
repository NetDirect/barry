///
/// \file	ostest.cc
///		Test application for the OpenSync API
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <iostream>
#include <stdexcept>
#include <memory>
#include <barry/barry.h>
#include "os22.h"
#include "os40.h"

using namespace std;
using namespace OpenSync;

void Test(API &os)
{
	cout << "=======================================================\n";
	cout << " Begin test run\n";
	cout << "=======================================================\n";

	cout << os.GetVersion() << endl;

	format_list_type flist;
	os.GetFormats(flist);
	cout << "Formats:\n" << flist << endl;

	string_list_type slist;
	os.GetPluginNames(slist);
	cout << "Plugins:\n" << slist << endl;

	os.GetGroupNames(slist);
	cout << "Groups:\n" << slist << endl;

	for( string_list_type::iterator b = slist.begin(); b != slist.end(); ++ b) {
		member_list_type mlist;
		os.GetMembers(*b, mlist);
		cout << "Members for group: " << *b << endl;
		cout << "---------------------------------------\n";
		cout << mlist << endl;
	}

	//
	// Test Group / Members
	//

	const std::string group_name = "ostest_trial_group";

	cout << "Testing with group_name: " << group_name << endl;

	// start fresh
	try { os.DeleteGroup(group_name); }
	catch( std::runtime_error &re ) {
		cout << "DeleteGroup: " << re.what() << endl;
	}

	// add group twice, to confirm behaviour
	os.AddGroup(group_name);
	cout << "Added: " << group_name << endl;

	try {
		os.AddGroup(group_name);
		throw std::logic_error("AddGroup() succeeded incorrectly!");
	}
	catch( std::runtime_error &re ) {
		cout << "AddGroup: " << re.what() << endl;
	}

	if( OpenSync40 *os40 = dynamic_cast<OpenSync40*>(&os) ) {
		try { os40->DeleteMember(group_name, 1); }
		catch( std::runtime_error &re ) {
			cout << "DeleteMember: " << re.what() << endl;
		}
	}

	// add member twice, to confirm behaviour
	os.AddMember(group_name, "barry-sync", "Barry sync member");
	os.AddMember(group_name, "evo2-sync", "Evolution sync member");
	os.AddMember(group_name, "file-sync", "File sync member");

	if( OpenSync40 *os40 = dynamic_cast<OpenSync40*>(&os) ) {
		os40->DeleteMember(group_name, "file-sync");
		try { os40->DeleteMember(group_name, "file-sync"); }
		catch( std::runtime_error &re ) {
			cout << "DeleteMember: " << re.what() << endl;
		}
	}

	// display our test group
	member_list_type mlist;
	os.GetMembers(group_name, mlist);
	cout << "Members for group: " << group_name << endl;
	cout << "---------------------------------------\n";
	cout << mlist << endl;

	// dump configurations
	cout << group_name << "(1): " << (os.IsConfigurable(group_name, 1) ?
			"configurable" : "not configurable") << endl;
	cout << group_name << "(2): " << (os.IsConfigurable(group_name, 2) ?
			"configurable" : "not configurable") << endl;

	cout << "Member 1's configuration:\n";
	cout << os.GetConfiguration(group_name, 1) << endl;
	cout << "Member 2's configuration:\n";
	cout << os.GetConfiguration(group_name, 2) << endl;

	// add comment to bottom of barry-sync config
	long id = mlist.FindId("barry-sync");
	string barry_config = os.GetConfiguration(group_name, id);
	if( dynamic_cast<OpenSync22*>(&os) )
		barry_config += "\n# This is a test\n";
	else
		barry_config += "<!-- This is a test -->\n";
	os.SetConfiguration(group_name, id, barry_config);
	cout << "New config for member " << id << ":\n";
	cout << os.GetConfiguration(group_name, id);

	// discover
	os.Discover(group_name);

	// delete group twice, to confirm behaviour
	os.DeleteGroup(group_name);
	cout << "Deleted: " << group_name << endl;

	try {
		os.DeleteGroup(group_name);
		throw std::logic_error("DeleteGroup() succeeded incorrectly!");
	}
	catch( std::runtime_error &re ) {
		cout << "DeleteGroup failed as expected" << endl;
	}

	cout << "=======================================================\n";
	cout << " End test run\n";
	cout << "=======================================================\n";
}

int main()
{
	Barry::Init(true);

	try {
		APISet set;
		set.OpenAvailable();

		if( set.os40() ) {
			Test(*set.os40());
		}

		if( set.os22() ) {
			Test(*set.os22());
		}
	}
	catch( std::exception &e ) {
		cout << "TEST FAILED: " << e.what() << endl;
	}

	return 0;
}

