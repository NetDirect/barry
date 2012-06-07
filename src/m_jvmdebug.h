///
/// \file	m_jvmdebug.h
///		Mode class for the JVMDebug mode
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008-2009, Nicolas VIVIEN

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

#ifndef __BARRY_M_JVMDEBUG_H__
#define __BARRY_M_JVMDEBUG_H__

#include "dll.h"
#include "m_mode_base.h"
#include "socket.h"
#include "record.h"
#include "data.h"

namespace Barry {

// forward declarations
class Parser;
class Builder;
class Controller;

class JVMModulesEntry;
class JVMThreadsEntry;


class BXEXPORT JVMModulesList : public std::vector<JVMModulesEntry>
{
public:
	typedef std::vector<JVMModulesEntry>		base_type;
	typedef base_type::iterator			iterator;
	typedef base_type::const_iterator		const_iterator;

public:
	void Parse(const Data &entry_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JVMModulesList &list) {
	list.Dump(os);
	return os;
}


class BXEXPORT JVMModulesEntry
{
public:
	uint32_t	Id;
	uint32_t	UniqueID;
	std::string	Name;

public:
	void Dump(std::ostream &os) const;
};


class BXEXPORT JVMThreadsList : public std::vector<JVMThreadsEntry>
{
public:
	typedef std::vector<JVMThreadsEntry>		base_type;
	typedef base_type::iterator			iterator;
	typedef base_type::const_iterator		const_iterator;

public:
	void Parse(const Data &entry_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JVMThreadsList &list) {
	list.Dump(os);
	return os;
}


class BXEXPORT JVMThreadsEntry
{
public:
	uint32_t	Id;
	uint8_t		Byte;
	uint32_t	Address;
	uint32_t	Unknown01;	// FIXME - perhaps should not expose
					// these to the app level, until we
					// have a name for them, since they
					// might change without notice
	uint32_t	Unknown02;
	uint32_t	Unknown03;
	uint32_t	Unknown04;
	uint32_t	Unknown05;
	uint32_t	Unknown06;

public:
	void Dump(std::ostream &os, int num) const;
};


namespace Mode {

//
// JVMDebug class
//
/// The main interface class to the java program debugger protocol
///
/// To use this class, use the following steps:
///
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::JVMDebug object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///
class BXEXPORT JVMDebug : public Mode
{
private:
	bool m_Attached;

protected:
	void ThrowJVMError(const std::string &msg, uint16_t cmd);

	//////////////////////////////////
	// overrides

	virtual void OnOpen();

public:
	JVMDebug(Controller &con);
	~JVMDebug();

	//////////////////////////////////
	// API
	void Close();

	void Attach();
	void Detach();

	// mid-attachment operations
	void Unknown01();	// FIXME - make these into a proper API, or
				// hide under protected:
	void Unknown02();
	void Unknown03();
	void Unknown04();
	void Unknown05();
	void Unknown06();
	void Unknown07();
	void Unknown08();
	void Unknown09();
	void Unknown10();
	void GetModulesList(JVMModulesList &mylist);
	void GetThreadsList(JVMThreadsList &mylist);
	int GetConsoleMessage(std::string &msg);
	bool GetStatus(int &status);
	bool WaitStatus(int &status);
	void Go();
	void Stop();
};

}} // namespace Barry::Mode

#endif

