///
/// \file	exechelper.h
///		Helper class to wrap wxProcess and wxExecute operations
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>

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

#ifndef __BARRYDESKTOP_EXECHELPER_H__
#define __BARRYDESKTOP_EXECHELPER_H__

#include <wx/wx.h>
#include <wx/process.h>

class ExecHelper;

class TermCatcher
{
	friend class ExecHelper;

	wxEvtHandler *m_handler;
	int m_id;
	ExecHelper *m_eh;

public:
	TermCatcher(wxEvtHandler *handler = 0, int id = 0)
		: m_handler(handler)
		, m_id(id)
		, m_eh(0)
	{
	}

	virtual ~TermCatcher();

	// WARNING: this can be called from another thread when not
	// using wxWidgets' exec code... it may not be possible to
	// safely call GUI functions from this callback  on all platforms.
	//
	// By default it just posts a wxProcessEvent(windowid, pid, status)
	// which is thread-safe.  Add a EVT_END_PROCESS(windowid, func)
	// handler to your event table to handle this.
	virtual void ExecTerminated(int pid, int status)
	{
		if( m_handler ) {
			wxProcessEvent pe(m_id, pid, status);
			m_handler->AddPendingEvent(pe);
		}
	}
};

//
// ExecHelper
//
/// Wrapper around the wxProcess class, that does much of the same
/// stuff, but also stores the child process's status code without
/// requiring a virtual override.
///
/// If ExecHelper is destroyed before the app exits, the callback
/// will be "lost", but the AppCallback class will cleanup after itself.
///
/// If you specify a TermCatcher pointer, then as long as TermCatcher
/// and ExecHelper exist, you will get an event via the virtual
/// ExecTerminated() call.  If either ExecHelper or TermCatcher
/// go out of scope before that happens, the callback will be "lost"
/// but everything will be cleaned up automatically.
///
/// The idea: Create a class where your real work happens, and derive
/// that class from TermCatcher.  Then create an instance
/// of ExecHelper inside that class, and pass 'this' to its
/// constructor.  If this is a dialog class that disappears
/// before the app exits, the destructors will handle things automatically.
///
class ExecHelper
{
	friend class TermCatcher;

protected:
	// This funky class is required because wxProcess deletes itself,
	// so that if ExecHelper is deleted before AppCallback, a segfault
	// is not caused by the OnTerminate() call.
	class AppCallback : public wxProcess
	{
		ExecHelper *m_container;
	public:
		AppCallback(ExecHelper *container)
			: m_container(container)
		{
		}

		void Detach()
		{
			m_container = 0;
		}

		// virtual overrides (wxProcess)
		virtual void OnTerminate(int pid, int status)
		{
			if( m_container && this == m_container->m_app_callback ) {
				m_container->m_app_pid = -1;
				m_container->m_app_status = status;
				m_container->m_app_callback = 0;

				// signal if legal
				if( m_container->m_catcher ) {
					try {
						m_container->m_catcher->ExecTerminated(pid, status);
					}
					catch( ... ) {
						// we are fully responsible
						// for cleaning ourselves up,
						// so do that, and then re-
						// throw, and hope for the
						// best...
						delete this;
						throw;
					}
				}
			}

			// cleanup
			delete this;
		}
	};

	TermCatcher *m_catcher;
	AppCallback *m_app_callback;
	int m_app_pid;
	int m_app_status;

protected:
	// helper functions
	void RunError(wxWindow *parent, const wxString &msg);
	int Execute(bool use_wx, const wxString &command, AppCallback *cb);

public:
	/// It is safe to pass a NULL catcher here.  No default is
	/// specified so that the compiler helps prevent forgetfulness.
	ExecHelper(TermCatcher *catcher);
	virtual ~ExecHelper();

	/// Runs the Application, if not already running.. parent may
	/// be NULL if you don't want this class to pop up error messages
	/// if unable to run the app.  The 'appname' argument is a
	/// user-friendly name for the application you are running.
	virtual bool Run(wxWindow *parent, const std::string &appname,
		const wxString &command);
	/// Returns true if App is currently running
	virtual bool IsAppRunning();
	virtual int GetAppStatus() const { return m_app_status; }
	/// Sends a termination signal to the App, if running
	virtual void KillApp(bool hardkill = false);
};

#endif

