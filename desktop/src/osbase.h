///
/// \file	osbase.h
///		Base API class for OpenSync interaction.
///		This API will operate both 0.22 and 0.4x
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

#ifndef __BARRYDESKTOP_OSBASE_H__
#define __BARRYDESKTOP_OSBASE_H__

#include <vector>
#include <string>
#include <iosfwd>

namespace OpenSync {

struct Member
{
	long id;
	std::string friendly_name;	// may not always have a name
	std::string plugin_name;
};

struct MemberSet : public std::vector<Member>
{
	Member* Find(const char *plugin_name);
	long FindId(const char *plugin_name); // returns -1 if not found
};

struct Format
{
	std::string name;
	std::string object_type;
};

struct FormatSet : public std::vector<Format>
{
	Format* Find(const char *name);
};

typedef std::vector<std::string>		string_list_type;
typedef MemberSet				member_list_type;
typedef FormatSet				format_list_type;

std::ostream& operator<< (std::ostream &os, const string_list_type &list);
std::ostream& operator<< (std::ostream &os, const Member &member);
std::ostream& operator<< (std::ostream &os, const member_list_type &list);
std::ostream& operator<< (std::ostream &os, const Format &format);
std::ostream& operator<< (std::ostream &os, const format_list_type &list);


class SyncConflictPrivateBase;

struct SyncChange
{
	int id;
	long member_id;
	std::string plugin_name;
	std::string uid;
	std::string printable_data;
};

class SyncConflict : public std::vector<SyncChange>
{
	SyncConflictPrivateBase &m_conflict;

public:
	SyncConflict(SyncConflictPrivateBase &conflict);
	~SyncConflict();

	bool IsAbortSupported() const;
	bool IsIgnoreSupported() const;
	bool IsKeepNewerSupported() const;

	std::string GetMenu() const;
	void Select(int change_id);  // takes the id field of SyncChange
	void Abort();
	void Duplicate();
	void Ignore();
	void KeepNewer();

	std::ostream& Dump(std::ostream &os) const;
};

inline std::ostream& operator<< (std::ostream &os, const SyncConflict &conflict)
{
	return conflict.Dump(os);
}


class SyncSummaryPrivateBase;

struct SyncMemberSummary
{
	int id;
	std::string objtype_name;
	long member_id;
	std::string plugin_name;
	unsigned long added;
	unsigned long modified;
	unsigned long deleted;

	SyncMemberSummary()
		: added(0), modified(0), deleted(0)
	{}
};

class SyncSummary : public std::vector<SyncMemberSummary>
{
	SyncSummaryPrivateBase &m_summary;

public:
	SyncSummary(SyncSummaryPrivateBase &summary);
	~SyncSummary();

	void Abort();
	void Continue();

	std::ostream& Dump(std::ostream &os) const;
};

inline std::ostream& operator<< (std::ostream &os, const SyncSummary &summary)
{
	return summary.Dump(os);
}


// notes: OpenSync::SyncStatus is a base class with all the opensync
// callbacks as virtual functions, with reasonable defaults.  The
// programmer can override any callbacks he so chooses as below.
//
// If a callback has state or information or requires a decision, it
// passes in a reference to a base class (example below: SyncConflict).
// This base class is really a reference to a derived class specific
// to the 0.22 or 0.4x library API, and contains pointers to the
// OpenSync40 or OpenSync22 classes and private structs, and handles
// all cleanup of the state it holds.  Also, these classes hold
// information in C++ style variables... for example SyncConflict
// will hold a vector of objects that contain the osync change
// information of each conflicting change, as well as a means to
// access a pretty printed version.  No OpenSync constants will
// be used in these objects.
//
// If abstracted enough, the override code should be dead simple,
// like below, and also be generic enough to run on both 0.22 and
// 0.4x libraries, dynamically. :-D
//
class SyncStatus
{
public:
	virtual ~SyncStatus();

	// virtual overrides
	virtual void HandleConflict(SyncConflict &conflict);
	virtual void EntryStatus(const std::string &msg, bool error);
	virtual void MappingStatus(const std::string &msg, bool error);
	virtual void EngineStatus(const std::string &msg, bool error);
	virtual void MemberStatus(long member_id,
		const std::string &plugin_name,
		const std::string &msg, bool error);
	virtual void CheckSummary(SyncSummary &summary);

	virtual void ReportError(const std::string &msg);
};

class API
{
public:
public:
	API()
	{
	}

	virtual ~API()
	{
	}

	// General engine information
	virtual const char* GetVersion() const = 0;
	virtual void GetPluginNames(string_list_type &plugins) = 0;
	virtual void GetFormats(format_list_type &formats) = 0;

	// Information about configured groups
	virtual void GetGroupNames(string_list_type &groups) = 0;
	virtual void GetMembers(const std::string &group_name,
		member_list_type &members) = 0;

	// Group configuration
	virtual void AddGroup(const std::string &group_name) = 0;
	virtual void DeleteGroup(const std::string &group_name) = 0;

	// Member configuration
	virtual void AddMember(const std::string &group_name,
		const std::string &plugin_name,
		const std::string &member_name) = 0;
	virtual bool IsConfigurable(const std::string &group_name,
		long member_id) = 0;
	virtual std::string GetConfiguration(const std::string &group_name,
		long member_id) = 0;
	virtual void SetConfiguration(const std::string &group_name,
		long member_id, const std::string &config_data) = 0;
	virtual void Discover(const std::string &group_name) = 0;

	// Syncing
	virtual void Sync(const std::string &group_name,
		SyncStatus &status_callback) = 0;
};

class APISet : private std::vector<API*>
{
	typedef std::vector<API*>		base_type;

public:
	APISet();
	~APISet();

	void OpenAll();		// throws if not all can be opened
	int OpenAvailable();	// opens only what is available and
				// returns # of APIs successfully loaded.
				// throws if some already loaded

	int GetAvailable() const;// returns # of APIs successfully loaded

	API* os40();
	API* os22();
};

} // namespace OpenSync

#endif

