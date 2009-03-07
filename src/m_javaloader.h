///
/// \file	m_javaloader.h
///		Mode class for the JavaLoader mode
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008-2009, Nicolas VIVIEN

        Some parts are inspired from m_desktop.h

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

#ifndef __BARRY_M_JAVALOADER_H__
#define __BARRY_M_JAVALOADER_H__

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
class CodFileBuilder;

class JLDirectoryEntry;

class JLEventlogEntry;

class BXEXPORT JLDirectory : public std::vector<JLDirectoryEntry>
{
public:
	typedef std::vector<JLDirectoryEntry>	BaseType;
	typedef BaseType::iterator		BaseIterator;
	typedef std::vector<uint16_t>		TableType;
	typedef TableType::iterator		TableIterator;

private:
	TableType m_idTable;

	int m_level;

public:
	JLDirectory(int level = 0);
	~JLDirectory();

	int Level() const { return m_level; }
	TableIterator TableBegin() { return m_idTable.begin(); }
	TableIterator TableEnd()   { return m_idTable.end(); }

	void ParseTable(const Data &table_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JLDirectory &d) {
	d.Dump(os);
	return os;
}

class BXEXPORT JLDirectoryEntry
{
private:
	int m_level;

public:
	uint16_t Id;
	std::string Name;
	std::string Version;
	uint32_t CodSize;
	time_t Timestamp;

	JLDirectory SubDir;

public:
	explicit JLDirectoryEntry(int level);

	void Parse(uint16_t id, const Data &entry_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JLDirectoryEntry &e) {
	e.Dump(os);
	return os;
}


class BXEXPORT JLScreenInfo {
public:
	uint16_t width;
	uint16_t height;

public:
	JLScreenInfo();
	~JLScreenInfo();
};


class BXEXPORT JLEventlog : public std::vector<JLEventlogEntry>
{
public:
	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JLEventlog &log) {
	log.Dump(os);
	return os;
}


class BXEXPORT JLEventlogEntry
{
public:
	typedef enum {
		ALWAYS_LOG,
		SEVERE_ERROR,
		ERROR,
		WARNING,
		INFORMATION,
		DEBUG_INFO
	} Severity_t;

	typedef enum {
		NUMBER = 1,
		STRING,
		EXCEPTION
	} ViewerType_t;

	std::string	Guid;
	uint64_t	MSTimestamp;	// time_t in milliseconds
	Severity_t	Severity;
	ViewerType_t	Type;
	std::string	App;
	std::string	Data;

protected:
	static Severity_t SeverityProto2Rec(unsigned int s);
	static unsigned int SeverityRec2Proto(Severity_t s);

	static ViewerType_t ViewerTypeProto2Rec(unsigned int v);
	static unsigned int ViewerTypeRec2Proto(ViewerType_t v);

public:
	void Parse(uint16_t size, const char* str);

	std::string GetFormattedTimestamp() const;

	void Dump(std::ostream &os) const;
};


class BXEXPORT JLDeviceInfo
{
public:
	struct VersionQuad {
		VersionQuad() { }
		VersionQuad(uint32_t v) {
			Major = (v & 0xff000000) >> 24;
			Minor = (v & 0xff0000) >> 16;
			SubMinor = (v & 0xff00) >> 8;
			Build = (v & 0xff);
		}

		unsigned int Major;
		unsigned int Minor;
		unsigned int SubMinor;
		unsigned int Build;
	};

public:
	uint32_t	HardwareId;
	uint32_t	Pin;
	VersionQuad	OsVersion;
	VersionQuad	VmVersion;
	uint32_t	RadioId;
	uint32_t	VendorId;
	uint32_t	ActiveWafs;
	Data		OsMetrics;
	Data		BootromMetrics;

public:
	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JLDeviceInfo &info) {
	info.Dump(os);
	return os;
}


namespace Mode {

//
// JavaLoader class
//
/// The main interface class to the java program loader protocol
///
/// To use this class, use the following steps:
///
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::JavaLoader object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///	- Call LoadDatabase() to retrieve and store a database
///
class BXEXPORT JavaLoader : public Mode
{
private:
	bool m_StreamStarted;

protected:
	void GetDirectoryEntries(JLPacket &packet, uint8_t entry_cmd,
		JLDirectory &dir, bool include_subdirs);
	void GetDir(JLPacket &packet, uint8_t entry_cmd, JLDirectory &dir,
		bool include_subdirs);
	void ThrowJLError(const std::string &msg, uint8_t cmd);
	void DoErase(uint8_t cmd, const std::string &cod_name);
	void SaveData(JLPacket &packet, uint16_t, CodFileBuilder &builder,
		std::ostream &output);

	//////////////////////////////////
	// overrides

	virtual void OnOpen();

public:
	JavaLoader(Controller &con);
	~JavaLoader();

	//////////////////////////////////
	// API
	void StartStream();
	bool StopStream();

	// mid-stream operations
	void SendStream(std::istream &input, size_t module_size);
	void LoadApp(std::istream &input);
	void SetTime(time_t when);
	void GetDirectory(JLDirectory &dir, bool include_subdirs);
	void GetScreenshot(JLScreenInfo &info, Data &image);
	void Erase(const std::string &cod_name);
	void ForceErase(const std::string &cod_name);
	void GetEventlog(JLEventlog &log);
	void ClearEventlog();
	void Save(const std::string &cod_name, std::ostream &output);
	void DeviceInfo(JLDeviceInfo &info);
	void Wipe(bool apps = true, bool fs = true);
	void LogStackTraces();
};

}} // namespace Barry::Mode

#endif

