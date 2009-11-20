///
/// \file	osbase.cc
///		Base API class helpers
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

#include "osbase.h"
#include "os22.h"
#include "os40.h"
#include "osprivatebase.h"
#include <iostream>
#include <iomanip>
#include <barry/barry.h>

using namespace std;

namespace OpenSync {

std::ostream& operator<< (std::ostream &os, const string_list_type &list)
{
	string_list_type::const_iterator b = list.begin(), e = list.end();
	for( ; b != e; ++b ) {
		os << *b << endl;
	}
	return os;
}

std::ostream& operator<< (std::ostream &os, const Member &member)
{
	os << "Member ID: 0x" << hex << member.id
	   << "\n   Plugin Name: " << member.plugin_name;
	os << "\n   Friendly Name: ";
	if( member.friendly_name.size() )
		os << member.friendly_name;
	else
		os << "<not set>";
	return os;
}

std::ostream& operator<< (std::ostream &os, const member_list_type &list)
{
	member_list_type::const_iterator b = list.begin(), e = list.end();
	for( ; b != e; ++b ) {
		os << *b << endl;
	}
	return os;
}

std::ostream& operator<< (std::ostream &os, const Format &format)
{
	os << "Format: " << format.name
	   << " (Object Type: " << format.object_type << ")";
	return os;
}

std::ostream& operator<< (std::ostream &os, const format_list_type &list)
{
	format_list_type::const_iterator b = list.begin(), e = list.end();
	for( ; b != e; ++b ) {
		os << *b << endl;
	}
	return os;
}


/////////////////////////////////////////////////////////////////////////////
// MemberSet public members

Member* MemberSet::Find(const char *plugin_name)
{
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( b->plugin_name == plugin_name )
			return &(*b);
	}
	return 0;
}

long MemberSet::FindId(const char *plugin_name)
{
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( b->plugin_name == plugin_name )
			return b->id;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// FormatSet public members

Format* FormatSet::Find(const char *name)
{
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( b->name == name )
			return &(*b);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// SyncConflict public members

SyncConflict::SyncConflict(SyncConflictPrivateBase &conflict)
	: m_conflict(conflict)
{
}

SyncConflict::~SyncConflict()
{
}

bool SyncConflict::IsAbortSupported() const
{
	return m_conflict.IsAbortSupported();
}

bool SyncConflict::IsIgnoreSupported() const
{
	return m_conflict.IsIgnoreSupported();
}

bool SyncConflict::IsKeepNewerSupported() const
{
	return m_conflict.IsKeepNewerSupported();
}

std::string SyncConflict::GetMenu() const
{
	ostringstream oss;
	oss << "Which entry do you want to use?\n[1-9] To select a side";

	if( IsAbortSupported() )
		oss << ", [A]bort";

	oss << ", [D]uplicate";

	if( IsIgnoreSupported() )
		oss << ", [I]gnore";

	if( IsKeepNewerSupported() )
		oss << ", Keep [N]ewer";

	return oss.str();
}

void SyncConflict::Select(int change_index)
{
	m_conflict.Select(change_index);
}

void SyncConflict::Abort()
{
	m_conflict.Abort();
}

void SyncConflict::Duplicate()
{
	m_conflict.Duplicate();
}

void SyncConflict::Ignore()
{
	m_conflict.Ignore();
}

void SyncConflict::KeepNewer()
{
	m_conflict.KeepNewer();
}

std::ostream& SyncConflict::Dump(std::ostream &os) const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		os << "Entry " << (b->id+1) << ":\n"
		   << "Member: " << b->member_id << "(" << b->plugin_name << ")\n"
		   << "UID: " << b->uid << "\n"
		   << "Data: " << b->printable_data << "\n";
	}
	return os;
}


/////////////////////////////////////////////////////////////////////////////
// SyncSummary public members

SyncSummary::SyncSummary(SyncSummaryPrivateBase &summary)
	: m_summary(summary)
{
}

SyncSummary::~SyncSummary()
{
}

void SyncSummary::Abort()
{
	m_summary.Abort();
}

void SyncSummary::Continue()
{
	m_summary.Continue();
}

std::ostream& SyncSummary::Dump(std::ostream &os) const
{
	string objtype_name;
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( b->objtype_name != objtype_name ) {
			objtype_name = b->objtype_name;
			os << "Objtype: " << b->objtype_name << "\n";
		}

		os << "\tMember " << b->id << "(" << b->member_id << ") "
		   << b->plugin_name
		   << ": Adding(" << b->added << ") "
		   << "Modifying(" << b->modified << ") "
		   << "Deleting(" << b->deleted << ")\n";
	}
	return os;
}


/////////////////////////////////////////////////////////////////////////////
// SyncStatus public members - default, CLI imeplementations

SyncStatus::~SyncStatus()
{
}

void SyncStatus::HandleConflict(SyncConflict &conflict)
{
	while( bool again = true ) {
		again = false;
		cout << "Conflicting items:\n" << conflict << endl;
		cout << conflict.GetMenu();
		string line;
		getline(cin, line);
		switch( line[0] )
		{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			conflict.Select(atoi(line.c_str()) - 1);
			break;
		case 'A':
			conflict.Abort();
			break;
		case 'D':
			conflict.Duplicate();
			break;
		case 'I':
			if( conflict.IsIgnoreSupported() )
				conflict.Ignore();
			break;
		case 'N':
			if( conflict.IsKeepNewerSupported() )
				conflict.KeepNewer();
			break;
		default:
			again = true;
			break;
		}
	}
}

void SyncStatus::EntryStatus(const std::string &msg, bool error)
{
	if( error )
		cout << "ERROR: ";
	cout << msg << endl;
}

void SyncStatus::MappingStatus(const std::string &msg, bool error)
{
	if( error )
		cout << "ERROR: ";
	cout << msg << endl;
}

void SyncStatus::EngineStatus(const std::string &msg, bool error)
{
	if( error )
		cout << "ERROR: ";
	cout << msg << endl;
}

void SyncStatus::MemberStatus(long member_id,
				const std::string &plugin_name,
				const std::string &msg,
				bool error)
{
	if( error )
		cout << "ERROR: ";
	cout << msg << endl;
}

void SyncStatus::CheckSummary(SyncSummary &summary)
{
	cout << "\nSynchronization Forecast Summary:\n";
	cout << summary << endl;

	cout << "Do you want to continue the synchronization? (N/y): ";
	string line;
	getline(cin, line);

	// Abort if not got accepted with 'y'
	if( line[0] != 'y') {
		cout << "\nAborting! Synchronization got aborted by user!" << endl;
		summary.Abort();
	} else {
		cout << "\nOK! Completing synchronization!" << endl;
		summary.Continue();
	}
}

void SyncStatus::ReportError(const std::string &msg)
{
	cout << "CALLBACK ERROR: " << msg << endl;
}


/////////////////////////////////////////////////////////////////////////////
// OpenSyncAPISet public members

APISet::APISet()
{
}

APISet::~APISet()
{
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		delete *b;
	}

	base_type::clear();
}

// throws if not all can be opened
void APISet::OpenAll()
{
	push_back( new OpenSync40 );
	push_back( new OpenSync22 );
}

// does not throw
int APISet::OpenAvailable()
{
	int loaded = 0;

	try {
		API *p = new OpenSync40;
		push_back(p);
		loaded++;
	}
	catch( std::exception &e ) {
		barryverbose("Unable to load opensync 0.40: " << e.what());
		push_back(0);
	}

	try {
		API *p = new OpenSync22;
		push_back(p);
		loaded++;
	}
	catch( std::exception &e ) {
		barryverbose("Unable to load opensync 0.22: " << e.what());
		push_back(0);
	}

	return loaded;
}

API* APISet::os40()
{
	return (*this)[0];
}

API* APISet::os22()
{
	return (*this)[1];
}

} // namespace OpenSync

