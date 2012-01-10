///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///		This header provides the common types and classes
///		used by the general record parser classes in the
///		r_*.h files.  Only application-safe API stuff goes in
///		here.  Internal library types go in record-internal.h
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_H__
#define __BARRY_RECORD_H__

#include "dll.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

// forward declarations
namespace Barry { class Data; }

namespace Barry {

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//


// stream-based wrapper to avoid printing strings that contain
// the \r carriage return characters
class BXEXPORT Cr2LfWrapper
{
	friend std::ostream& operator<< (std::ostream &os, const Cr2LfWrapper &str);
	const std::string &m_str;
public:
	explicit Cr2LfWrapper(const std::string &str)
		: m_str(str)
	{
	}
};
BXEXPORT std::ostream& operator<< (std::ostream &os, const Cr2LfWrapper &str);

struct BXEXPORT CommandTableCommand
{
	unsigned int Code;
	std::string Name;
};

class BXEXPORT CommandTable
{
public:
	typedef CommandTableCommand Command;
	typedef std::vector<Command> CommandArrayType;

	CommandArrayType Commands;

private:
	BXLOCAL const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
public:
	CommandTable();
	~CommandTable();

	void Parse(const Data &data, size_t offset);
	void Clear();

	// returns 0 if unable to find command name, which is safe, since
	// 0 is a special command that shouldn't be in the table anyway
	unsigned int GetCommand(const std::string &name) const;

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const CommandTable &command) {
	command.Dump(os);
	return os;
}



struct BXEXPORT RecordStateTableState
{
	unsigned int Index;
	uint32_t RecordId;
	bool Dirty;
	unsigned int RecType;
	std::string Unknown2;
};

class BXEXPORT RecordStateTable
{
public:
	typedef RecordStateTableState State;
	typedef unsigned int IndexType;
	typedef std::map<IndexType, State> StateMapType;

	StateMapType StateMap;

private:
	mutable IndexType m_LastNewRecordId;

private:
	BXLOCAL const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	RecordStateTable();
	~RecordStateTable();

	void Parse(const Data &data);
	void Clear();

	bool GetIndex(uint32_t RecordId, IndexType *pFoundIndex = 0) const;
	uint32_t MakeNewRecordId() const;

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const RecordStateTable &rst) {
	rst.Dump(os);
	return os;
}



struct BXEXPORT DatabaseItem
{
	unsigned int Number;
	unsigned int RecordCount;
	std::string Name;
};

class BXEXPORT DatabaseDatabase
{
public:
	typedef DatabaseItem Database;
	typedef std::vector<Database> DatabaseArrayType;

	DatabaseArrayType Databases;

private:
	template <class RecordType, class FieldType>
	void ParseRec(const RecordType &rec, const unsigned char *end);

	template <class FieldType>
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	DatabaseDatabase();
	~DatabaseDatabase();

	void Parse(const Data &data);
	void Clear();

	void SortByName();
	unsigned int GetTotalRecordCount() const;

	// returns true on success, and fills target
	bool GetDBNumber(const std::string &name, unsigned int &number) const;
	bool GetDBName(unsigned int number, std::string &name) const;

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const DatabaseDatabase &dbdb) {
	dbdb.Dump(os);
	return os;
}

struct UnknownData
{
	std::string raw_data;

	const std::string::value_type* data() const { return raw_data.data(); }
	std::string::size_type size() const { return raw_data.size(); }
	void assign(const std::string::value_type *s, std::string::size_type n)
		{ raw_data.assign(s, n); }
};

struct BXEXPORT UnknownField
{
	uint8_t type;
	UnknownData data;
};
typedef std::vector<UnknownField> UnknownsType;
BXEXPORT std::ostream& operator<< (std::ostream &os, const UnknownsType &unknowns);

struct BXEXPORT EmailAddress
{
	std::string Name;
	std::string Email;

	EmailAddress()
	{
	}

	/// Converts "Name <address@host.com>" into Name + Address
	/// Will also handle just a plain address too.
	explicit EmailAddress(const std::string &complex_address);

	void clear()
	{
		Name.clear();
		Email.clear();
	}

	size_t size() const
	{
		return Name.size() + Email.size();
	}
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const EmailAddress &msga);

class BXEXPORT EmailAddressList : public std::vector<EmailAddress>
{
public:
	std::string ToCommaSeparated() const;
	void AddCommaSeparated(const std::string &list);
};

BXEXPORT std::ostream& operator<<(std::ostream &os, const EmailAddressList &elist);

struct BXEXPORT PostalAddress
{
	std::string
		Address1,
		Address2,
		Address3,
		City,
		Province,
		PostalCode,
		Country;

	std::string GetLabel() const;
	void Clear();

	bool HasData() const { return Address1.size() || Address2.size() ||
		Address3.size() || City.size() || Province.size() ||
		PostalCode.size() || Country.size(); }
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const PostalAddress &msga);

struct BXEXPORT Date
{
	int Month;			// 0 to 11
	int Day;			// 1 to 31
	int Year;			// exact number, eg. 2008

	Date() : Month(0), Day(0), Year(0) {}
	explicit Date(const struct tm *timep);

	bool HasData() const { return Month || Day || Year; }
	void Clear();

	void ToTm(struct tm *timep) const;
	std::string ToYYYYMMDD() const;
	std::string ToBBString() const;	// converts to Blackberry string
					// format of DD/MM/YYYY

	bool FromTm(const struct tm *timep);
	bool FromBBString(const std::string &str);
	bool FromYYYYMMDD(const std::string &str);
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const Date &date);

class BXEXPORT CategoryList : public std::vector<std::string>
{
public:
	/// Parses the given comma delimited category string into
	/// this CategoryList object, appending each token to the vector.
	/// Will clear vector beforehand.
	void CategoryStr2List(const std::string &str);

	/// Turns the current vectory into a comma delimited category
	/// string suitable for use in Calendar, Task, and Memo
	/// protocol values.
	void CategoryList2Str(std::string &str) const;

	using std::vector<std::string>::operator=;
};


/// \addtogroup RecordParserClasses
///		Parser and data storage classes.  These classes take a
///		Database Database record and convert them into C++ objects.
///		Each of these classes are safe to be used in standard
///		containers, and are meant to be used in conjunction with the
///		RecordParser<> template when calling Controller::LoadDatabase().
/// @{
/// @}

} // namespace Barry

#ifndef __BARRY_LIBRARY_BUILD__
// Include all parser classes, to make it easy for the application to use.
#include "r_calendar.h"
#include "r_calllog.h"
#include "r_bookmark.h"
#include "r_contact.h"
#include "r_cstore.h"
#include "r_memo.h"
#include "r_message.h"
#include "r_servicebook.h"
#include "r_task.h"
#include "r_pin_message.h"
#include "r_saved_message.h"
#include "r_sms.h"
#include "r_folder.h"
#include "r_timezone.h"
#include "r_hhagent.h"
#endif

#endif

