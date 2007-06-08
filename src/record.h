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
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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



class CommandTable
{
public:
	struct Command
	{
		unsigned int Code;
		std::string Name;
	};

	typedef std::vector<Command> CommandArrayType;

	CommandArrayType Commands;

private:
	const unsigned char* ParseField(const unsigned char *begin,
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

inline std::ostream& operator<< (std::ostream &os, const CommandTable &command) {
	command.Dump(os);
	return os;
}



class RecordStateTable
{
public:
	struct State
	{
		unsigned int Index;
		uint32_t RecordId;
		bool Dirty;
		unsigned int RecType;
		std::string Unknown2;
	};

	typedef unsigned int IndexType;
	typedef std::map<IndexType, State> StateMapType;

	StateMapType StateMap;

private:
	mutable IndexType m_LastNewRecordId;

private:
	const unsigned char* ParseField(const unsigned char *begin,
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

inline std::ostream& operator<< (std::ostream &os, const RecordStateTable &rst) {
	rst.Dump(os);
	return os;
}



class DatabaseDatabase
{
public:
	struct Database
	{
		unsigned int Number;
		unsigned int RecordCount;
		std::string Name;
	};

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

	// returns true on success, and fills target
	bool GetDBNumber(const std::string &name, unsigned int &number) const;
	bool GetDBName(unsigned int number, std::string &name) const;

	void Dump(std::ostream &os) const;
};

inline std::ostream& operator<<(std::ostream &os, const DatabaseDatabase &dbdb) {
	dbdb.Dump(os);
	return os;
}

struct UnknownField
{
	uint8_t type;
	std::string data;
};
std::ostream& operator<< (std::ostream &os, const std::vector<UnknownField> &unknowns);

struct Address
{
	std::string Name;
	std::string Email;

	void clear()
	{
		Name.clear();
		Email.clear();
	}
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

// Include all parser classes, to make it easy for the application to use.
#include "r_calendar.h"
#include "r_contact.h"
#include "r_memo.h"
#include "r_message.h"
#include "r_servicebook.h"
#include "r_task.h"
#include "r_pin_message.h"
#include "r_saved_message.h"

#endif

