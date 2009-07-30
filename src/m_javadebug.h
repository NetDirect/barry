///
/// \file	m_javadebug.h
///		Mode class for the JavaDebug mode
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
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

#ifndef __BARRY_M_JAVADEBUG_H__
#define __BARRY_M_JAVADEBUG_H__

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

class JDModulesEntry;
class JDThreadsEntry;


class BXEXPORT JDModulesList : public std::vector<JDModulesEntry>
{
public:
	void Parse(const Data &entry_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JDModulesList &list) {
	list.Dump(os);
	return os;
}


class BXEXPORT JDModulesEntry
{
public:
	uint32_t	Id;
	uint32_t	Address;
	std::string	Name;

public:
	void Dump(std::ostream &os) const;
};


class BXEXPORT JDThreadsList : public std::vector<JDThreadsEntry>
{
public:
	void Parse(const Data &entry_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JDThreadsList &list) {
	list.Dump(os);
	return os;
}


class BXEXPORT JDThreadsEntry
{
public:
	uint32_t	Id;
	uint8_t		Byte;
	uint32_t	Address;
	uint32_t	Unknown01;
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
// JavaDebug class
//
/// The main interface class to the java program debugger protocol
///
/// To use this class, use the following steps:
///
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::JavaDebug object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///
class BXEXPORT JavaDebug : public Mode
{
private:
	bool m_Attached;

protected:
	void ThrowJDError(const std::string &msg, uint8_t cmd);

	//////////////////////////////////
	// overrides

	virtual void OnOpen();

public:
	JavaDebug(Controller &con);
	~JavaDebug();

	//////////////////////////////////
	// API
	void Attach();
	void Detach();

	// mid-attachment operations
	void Unknown01();
	void Unknown02();
	void Unknown03();
	void Unknown04();
	void Unknown05();
	void Unknown06();
	void Unknown07();
	void Unknown08();
	void Unknown09();
	void Unknown10();
	void GetModulesList(JDModulesList &mylist);
	void GetThreadsList(JDThreadsList &mylist);
	int GetConsoleMessage(std::string &msg);
	void GetStatus(int &status);
	void Go();
	void Stop();
};

}} // namespace Barry::Mode

#endif

