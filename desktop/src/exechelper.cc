///
/// \file	exechelper.cc
///		Helper class to wrap wxProcess and wxExecute operations
///

/*
    Copyright (C) 2010-2012, Chris Frey <cdfrey@foursquare.net>

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

#include "exechelper.h"

#include <wx/tokenzr.h>

#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

TermCatcher::~TermCatcher()
{
	if( m_eh )
		m_eh->m_catcher = 0;
}

ExecHelper::ExecHelper(TermCatcher *catcher)
	: m_catcher(catcher)
	, m_app_callback(0)
	, m_app_pid(-1)
	, m_app_status(-1)
	, m_started(0)
{
	// link ourselves to the catcher... the catcher will unlink if necessary
	if( m_catcher )
		m_catcher->m_eh = this;
}

ExecHelper::~ExecHelper()
{
	if( m_app_callback ) {
		m_app_callback->Detach();
		m_app_callback = 0;
	}
	if( m_catcher )
		m_catcher->m_eh = 0;
}

void ExecHelper::RunError(wxWindow *parent, const wxString &msg)
{
	if( !parent )
		return;

	wxMessageBox(msg, _T("Application Run Error"),
		wxOK | wxICON_ERROR, parent);
}

int ExecHelper::Execute(bool use_wx,
			const wxString &command,
			AppCallback *cb)
{
	if( use_wx ) {
		return wxExecute(command, wxEXEC_ASYNC, m_app_callback);
	}

	//
	// use our own forking mechanism, due to bugs in wxWidgets :-(
	//

	class WaitThread : public wxThread
	{
		int m_pid;
		AppCallback *m_callback;

	public:
		WaitThread(int pid, AppCallback *cb)
			: m_pid(pid)
			, m_callback(cb)
		{
		}

		virtual void* Entry()
		{
			int status;
			int pid = waitpid(m_pid, &status, 0);
			if( pid == m_pid ) {
				// our child finished
				m_callback->OnTerminate(pid, status);
			}

			return 0;
		}
	};

	// about to fork, log the start time
	m_started = time(NULL);

	// create child
	int pid = fork();
	if( pid == -1 ) {
		// no child created
		return -1;
	}
	else if( pid == 0 ) {
		// we are the child

		// parse the command line into an array
		char *argv[100];
		int argc = 0;
		wxStringTokenizer t(command, _T(" "));
		while( t.HasMoreTokens() && argc < 99 ) {
			wxString token = t.GetNextToken();
			std::string ctoken(token.utf8_str());
			argv[argc] = new char[ctoken.size() + 1];
			strcpy(argv[argc], ctoken.c_str());
			argc++;
		}
		argv[argc] = 0;

		execvp(argv[0], argv);

		cerr << "execvp() failed: " << strerror(errno) << endl;
		for( int i = 0; argv[i]; i++ ) {
			cerr << argv[i] << " ";
		}
		cerr << endl;

		exit(255);
	}
	else {
		// we are the parent... start the wait thread
		WaitThread *wt = new WaitThread(pid, cb);
		wt->Create();
		wt->Run();
		return pid;
	}
}

bool ExecHelper::Run(wxWindow *parent,
			const std::string &appname,
			const wxString &command)
{
	if( IsAppRunning() ) {
		RunError(parent, wxString(appname.c_str(), wxConvUTF8) +
			_T(" is already running."));
		return false;
	}

	m_app_callback = new AppCallback(this);
	m_app_pid = Execute(false, command, m_app_callback);
	if( m_app_pid <= 0 ) {
		delete m_app_callback;
		m_app_callback = 0;
		m_app_pid = -1;

		RunError(parent, _T("Failed to run ") +
			wxString(appname.c_str(), wxConvUTF8) +
			_T(". Please make sure it is installed and in your PATH."));
		return false;
	}

	return true;
}

bool ExecHelper::IsAppRunning()
{
	return m_app_callback && m_app_pid > 0;
}

void ExecHelper::WaitForChild()
{
	while( IsAppRunning() )
		usleep(50000);
}

int ExecHelper::GetChildExitCode() const
{
	int status = GetRawAppStatus();
	return WEXITSTATUS(status);
}

void ExecHelper::KillApp(bool hardkill)
{
	if( IsAppRunning() ) {
//		m_app_callback->Kill(m_app_pid, hardkill ? wxSIGKILL : wxSIGTERM);
		kill(m_app_pid, hardkill ? SIGKILL : SIGTERM);
		// let the callback handle the cleanup
	}
}

