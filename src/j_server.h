///
/// \file	server.h
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
private:
	int fd;
	int sockfd;

	std::string address;
	int port;

	bool loop;
	bool targetrunning;

	std::string password;
	Barry::Mode::JVMDebug *jvmdebug;

	Barry::JVMModulesList modulesList;				// List of COD applications installed on the device
	Barry::JDG::JDGDebugFileList debugFileList;		// List of debug file on the host

	JDWAppList appList;								// List of BlackBerry application (an application contents several COD files)
	Barry::JDG::JDGClassList visibleClassList;		// Visible class list from JDB

	std::auto_ptr<Thread> handler;
	void (*printConsoleMessage)(std::string message);

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
	JDWServer(const char *address, int port);
	~JDWServer();

	void setDevice(Barry::Mode::JVMDebug *device);
	void setPasswordDevice(std::string password);

	void setConsoleCallback(void (*callback)(std::string message));

	bool start();
	void acceptConnection();
	void attachToDevice();
	void initVisibleClassList();
	bool hello();
	void run();
	void detachFromDevice();
	bool stop();
};

}} // namespace Barry::JDWP

#endif

