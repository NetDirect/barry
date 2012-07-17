///
/// \file	bsyncjail.cc
///		Helper program to isolate the actual syncing into its
///		own process.  Communicates with the main barrydesktop
///		via wxWidgets IPC communication.  This belongs in its
///		own process since the sync can hang, and may need to
///		be killed from the GUI.
///

/*
    Copyright (C) 2010-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <sstream>
#include <stdlib.h>
#include "os22.h"
#include "os40.h"
#include "tempdir.h"
#include "ostypes.h"
#include "i18n.h"

using namespace std;

// command line arguments
string g_argv_version, g_argv_group_name;
OpenSync::Config::pst_type g_sync_types = PST_DO_NOT_SET;

class BarrySyncJail : public OpenSync::SyncStatus
{
private:
	TempDir m_temp;
	std::auto_ptr<OpenSync::API> m_engine;

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
//	virtual void CheckSummary(OpenSync::SyncSummary &summary);
};

//////////////////////////////////////////////////////////////////////////////
// BarrySyncJail

BarrySyncJail::BarrySyncJail()
	: m_temp("bsyncjail")
{
	OnInit();
}

BarrySyncJail::~BarrySyncJail()
{
}

bool BarrySyncJail::OnInit()
{
	cerr << "OnInit()" << endl;

	// load opensync engine
	try {
		if( g_argv_version == "0.22" )
			m_engine.reset( new OpenSync::OpenSync22 );
		else {
			m_engine.reset( new OpenSync::OpenSync40 );
		}
	}
	catch( std::exception &e ) {
		cerr << e.what() << endl;
		return false;
	}

	if( !m_engine.get() ) {
		cerr << "Unknown engine number: " << g_argv_version << endl;
		return false;
	}

	// start the sync
	try {
		m_engine->Discover(g_argv_group_name);
		m_engine->Sync(g_argv_group_name, *this, g_sync_types);
	}
	catch( std::exception &e ) {
		cerr << e.what() << endl;
		return true;
	}

	return true;
}

int BarrySyncJail::OnExit()
{
	cerr << "OnExit()" << endl;

	return 0;
}

//
// OpenSync::SyncStatus virtual overrides
//

/*
void BarrySyncJail::CheckSummary(OpenSync::SyncSummary &summary)
{
	cerr << "CheckSummary" << endl;
	// FIXME: not currently supported... abort every time
//	cerr << "FIXME: CheckSummary() not implemented, aborting" << endl;
//	summary.Abort();
	summary.Continue();
}
*/

int main(int argc, char *argv[])
{
	cerr << "bsyncjail startup" << endl;

	if( argc != 4 ) {
		cerr << _C("This is a helper program for barrydesktop, and\n"
			"is not intended to be called directly.\n") << endl;
		return 1;
	}

	g_argv_version = argv[1];
	g_argv_group_name = argv[2];
	g_sync_types = atoi(argv[3]);

	BarrySyncJail app;
	return app.OnExit();
}

