///
/// \file	bsyncjail.cc
///		Helper program to isolate the actual syncing into its
///		own process.  Communicates with the main barrydesktop
///		via wxWidgets IPC communication.  This belongs in its
///		own process since the sync can hang, and may need to
///		be killed from the GUI.
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <wx/wx.h>
#include <iostream>
#include <sstream>
#include "ipc.h"
#include "os22.h"
#include "os40.h"
#include "tempdir.h"
#include "ostypes.h"
#include "wxi18n.h"

using namespace std;

// command line arguments
string g_argv_version, g_argv_group_name;
OpenSync::Config::pst_type g_sync_types = PST_DO_NOT_SET;
SillyBuffer sb;

class ClientConnection : public wxConnection
{
public:
	ClientConnection() {}
};

class Client : public wxClient
{
public:
	Client() {}
	ClientConnection* OnMakeConnection()
	{
		return new ClientConnection;
	}
};

class BarrySyncJail : public OpenSync::SyncStatus
{
private:
	TempDir m_temp;
	std::auto_ptr<OpenSync::API> m_engine;
	std::auto_ptr<wxClient> m_client;
	wxConnectionBase *m_status_con, *m_conflict_con;

	// communication variables
	int m_sequenceID;
public:
	BarrySyncJail();
	~BarrySyncJail();

	//
	// overrides
	//
	virtual bool OnInit();
	virtual int OnExit();

	//
	// OpenSync::SyncStatus virtual overrides
	//
	virtual void HandleConflict(OpenSync::SyncConflict &conflict);
	virtual void EntryStatus(const std::string &msg, bool error);
	virtual void MappingStatus(const std::string &msg, bool error);
	virtual void EngineStatus(const std::string &msg, bool error,
		bool slowsync);
	virtual void MemberStatus(long member_id,
		const std::string &plugin_name,
		const std::string &msg, bool error);
	virtual void CheckSummary(OpenSync::SyncSummary &summary);
	virtual void ReportError(const std::string &msg);
};

DECLARE_APP(BarrySyncJail)

//////////////////////////////////////////////////////////////////////////////
// BarrySyncJail

BarrySyncJail::BarrySyncJail()
	: m_temp("bsyncjail")
	, m_status_con(0)
	, m_conflict_con(0)
	, m_sequenceID(0)
{
	OnInit();
}

BarrySyncJail::~BarrySyncJail()
{
}

bool BarrySyncJail::OnInit()
{
	cerr << "OnInit()" << endl;

	// connect to parent app
	m_client.reset( new Client );
	m_status_con = m_client->MakeConnection(_T("localhost"),
		SERVER_SERVICE_NAME, TOPIC_STATUS);
	m_conflict_con = m_client->MakeConnection(_T("localhost"),
		SERVER_SERVICE_NAME, TOPIC_CONFLICT);
	if( !m_status_con || !m_conflict_con ) {
		cerr << _C("Unable to connect to server.") << endl;
		return false;
	}

	// load opensync engine
	try {
		if( g_argv_version.substr(0, 3) == "0.2" )
			m_engine.reset( new OpenSync::OpenSync22 );
		else {
			m_engine.reset( new OpenSync::OpenSync40 );
			if( g_argv_version != m_engine->GetVersion() )
				throw std::runtime_error(_C("Can't find matching engine for: ") + g_argv_version);
		}
	}
	catch( std::exception &e ) {
		m_status_con->Poke(STATUS_ITEM_ERROR, sb.buf(e.what()));
		cerr << e.what() << endl;
		return false;
	}

	if( !m_engine.get() ) {
		m_status_con->Poke(STATUS_ITEM_ERROR, sb.buf(string(_C("Unknown engine number: ")) + g_argv_version));
		return false;
	}

	// start the sync
	try {
		m_engine->Discover(g_argv_group_name);
		m_engine->Sync(g_argv_group_name, *this, g_sync_types);
	}
	catch( std::exception &e ) {
		m_status_con->Poke(STATUS_ITEM_ERROR, sb.buf(e.what()));
		cerr << e.what() << endl;
		return true;
	}

	return true;
}

int BarrySyncJail::OnExit()
{
	cerr << "OnExit()" << endl;

	// clean up the client connection... do this early, so that
	// TempDir can clean up the files if necessary
	m_client.reset();

	return 0;
}

//
// OpenSync::SyncStatus virtual overrides
//

void BarrySyncJail::HandleConflict(OpenSync::SyncConflict &conflict)
{
	OpenSync::SyncConflict::iterator i;
	int size = 0;
	wxChar *buf = 0;

	// start with a new sequence ID
	m_sequenceID++;
	int offset = 0;

	// msg 1: sequence ID, offset 0
	//	send available menu / functions possible, and number of
	//	changes that conflict

	ostringstream oss;
	oss << m_sequenceID << " " << offset << " " << conflict.size()
		<< " " << "SD";
	if( conflict.IsAbortSupported() )
		oss << "A";
	if( conflict.IsIgnoreSupported() )
		oss << "I";
	if( conflict.IsKeepNewerSupported() )
		oss << "N";

	if( !m_conflict_con->Poke(CONFLICT_ITEM_START, sb.buf(oss.str())) )
		goto connection_lost;

	// all following messages contain the sequence ID,
	// the change ID, and the change data
	i = conflict.begin();
	for( ; i != conflict.end(); ++i ) {
		oss.str("");
		offset++;

		oss << m_sequenceID << " " << offset << " " << i->id << " \n";
		oss << i->plugin_name << "\n";
		oss << i->uid << "\n";
		oss << i->printable_data;

		if( !m_conflict_con->Poke(CONFLICT_ITEM_CHANGE, sb.buf(oss.str())) )
			goto connection_lost;
	}

	// then wait on the server to tell us what choice was made
	buf = m_conflict_con->Request(CONFLICT_ITEM_ANSWER, &size);
	if( buf ) {
		wxString msg(buf);
		istringstream iss(string(msg.utf8_str()));
		int sequenceID = 0;
		string command;
		int id;

		iss >> sequenceID >> command;
		if( !iss || sequenceID != m_sequenceID ) {
			// invalid command from server, something is wrong
			throw std::runtime_error(_C("Invalid server response: ") + string(msg.utf8_str()));
		}

		switch( command[0] )
		{
		case 'S':	// Select
			iss >> id;
			if( !iss )
				throw std::runtime_error(_C("Invalid Select command from server: ") + string(msg.utf8_str()));
			conflict.Select(id);
			break;

		case 'D':	// Duplicate
			conflict.Duplicate();
			break;

		case 'A':	// Abort
			if( !conflict.IsAbortSupported() )
				throw std::runtime_error(_C("Abort not supported, and server sent Abort command."));
			conflict.Abort();
			break;

		case 'I':	// Ignore
			if( !conflict.IsIgnoreSupported() )
				throw std::runtime_error(_C("Ignore not supported, and server sent Ignore command."));
			conflict.Ignore();
			break;

		case 'N':	// Keep Newer
			if( !conflict.IsKeepNewerSupported() )
				throw std::runtime_error(_C("Keep Newer not supported, and server sent Keep Newer command."));
			conflict.KeepNewer();
			break;

		default:
			throw std::runtime_error(_C("Invalid command from server: ") + string(msg.utf8_str()));
		}

		// all done!
		return;
	}

connection_lost:
	if( conflict.IsAbortSupported() )
		conflict.Abort();
	else
		throw std::runtime_error(_C("Lost connection with server"));
}

void BarrySyncJail::EntryStatus(const std::string &msg, bool error)
{
	if( error )
		ReportError(msg);
	else
		m_status_con->Poke(STATUS_ITEM_ENTRY, sb.buf(msg));
}

void BarrySyncJail::MappingStatus(const std::string &msg, bool error)
{
	if( error )
		ReportError(msg);
	else
		m_status_con->Poke(STATUS_ITEM_MAPPING, sb.buf(msg));
}

void BarrySyncJail::EngineStatus(const std::string &msg, bool error, bool slowsync)
{
	if( error )
		ReportError(msg);
	else
		m_status_con->Poke(STATUS_ITEM_ENGINE, sb.buf(msg));

	// slow sync on 0.22 is unreliable... send a special notice
	// to the GUI
	if( slowsync && !m_engine->IsSlowSyncSupported() )
		m_status_con->Poke(STATUS_ITEM_ENGINE, sb.buf(ENGINE_STATUS_SLOW_SYNC));
}

void BarrySyncJail::MemberStatus(long member_id,
				const std::string &plugin_name,
				const std::string &msg, bool error)
{
	if( error )
		ReportError(msg);
	else
		m_status_con->Poke(STATUS_ITEM_MEMBER, sb.buf(msg));
}

void BarrySyncJail::CheckSummary(OpenSync::SyncSummary &summary)
{
	// FIXME: not currently supported... abort every time
//	cerr << "FIXME: CheckSummary() not implemented, aborting" << endl;
//	summary.Abort();
	summary.Continue();
}

void BarrySyncJail::ReportError(const std::string &msg)
{
	m_status_con->Poke(STATUS_ITEM_ERROR, sb.buf(msg));
	cerr << "ReportError(): " << msg << endl;
}


int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	wxApp::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, "bsyncjail");

	cerr << "bsyncjail startup" << endl;

	if( argc != 4 ) {
		cerr << _C("This is a helper program for barrydesktop, and\n"
			"is not intended to be called directly.\n") << endl;
		return 1;
	}

	g_argv_version = argv[1];
	g_argv_group_name = argv[2];
	g_sync_types = atoi(argv[3]);

	wxInitializer initializer;
	if( !initializer ) {
		cerr << _C("Unable to initialize wxWidgets library, aborting.") << endl;
		return 1;
	}

	BarrySyncJail app;
	int ret = app.OnExit();
	cerr << _C("bsyncjail exiting with code: ") << ret << endl;
	return ret;
}

