///
/// \file	j_server.h
///		Java Debug server classes
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

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

#ifndef __BARRYJDWP_SERVER_H__
#define __BARRYJDWP_SERVER_H__

#include "dll.h"
#include "j_manager.h"
#include "dp_codinfo.h"
#include "m_jvmdebug.h"
#include "threadwrap.h"
#include <string>
#include <memory>


namespace Barry { namespace JDWP {

class BXEXPORT JDWServer
{
public:
	typedef void (*ConsoleCallbackType)(const std::string &);

private:
	Barry::Mode::JVMDebug *jvmdebug;

	int acceptfd;
	int sockfd;

	std::string address;
	int port;

	bool loop;
	bool targetrunning;

	std::string password;

	Barry::JVMModulesList modulesList;				// List of COD applications installed on the device
	Barry::JDG::DebugFileList debugFileList;		// List of debug file on the host

	JDWAppList appList;								// List of BlackBerry application (an application contents several COD files)
	Barry::JDG::ClassList visibleClassList;		// Visible class list from JDB

	std::auto_ptr<Thread> handler;
	ConsoleCallbackType printConsoleMessage;

	void CommandsetProcess(Barry::Data &cmd);

	void CommandsetVirtualMachineProcess(Barry::Data &cmd);
	void CommandsetEventRequestProcess(Barry::Data &cmd);

	void CommandVersion(Barry::Data &cmd);
	void CommandIdSizes(Barry::Data &cmd);
	void CommandAllClasses(Barry::Data &cmd);
	void CommandAllThreads(Barry::Data &cmd);
	void CommandSuspend(Barry::Data &cmd);
	void CommandResume(Barry::Data &cmd);
	void CommandClassPaths(Barry::Data &cmd);

	void CommandSet(Barry::Data &cmd);

//	void BackgroundDeviceProcess();

protected:

public:
	JDWServer(Barry::Mode::JVMDebug &device, const char *address, int port);
	~JDWServer();

	void SetPasswordDevice(std::string password);

	void SetConsoleCallback(ConsoleCallbackType callback);

	bool Start();
	void AcceptConnection();
	void AttachToDevice();
	void InitVisibleClassList();
	bool Hello();
	void Run();
	void DetachFromDevice();
	bool Stop();
};

}} // namespace Barry::JDWP

#endif

