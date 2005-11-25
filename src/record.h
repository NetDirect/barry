///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <stdint.h>

#include "protocol.h"			// only needed for size typedefs

// forward declarations
class Data;

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

	void Parse(const Data &data, int offset);
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

	// FIXME - returns 0 on error here, but that's a valid DBNumber
	unsigned int GetDBNumber(const std::string &name) const;

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


/// \addtogroup RecordParserClasses
///		Parser and data storage classes.  These classes take a
///		Database Database record and convert them into C++ objects.
///		Each of these classes are safe to be used in standard
///		containers, and are meant to be used in conjunction with the
///		RecordParser<> template when calling Controller::LoadDatabase().
/// @{

class Contact
{
public:
	// protocol record types, for size calculations
	typedef Barry::OldContactRecord		OldProtocolRecordType;
	typedef Barry::ContactRecord		ProtocolRecordType;

private:
	// private contact management data
	uint64_t m_recordId;

public:
	// contact specific data
	std::string
		Email,
		Phone,
		Fax,
		WorkPhone,
		HomePhone,
		MobilePhone,
		Pager,
		PIN,
		FirstName,
		LastName,
		Company,
		DefaultCommunicationsMethod,
		Address1,
		Address2,
		Address3,
		City,
		Province,
		PostalCode,
		Country,
		Title,
		PublicKey,
		Notes;

	std::vector<uint64_t> GroupLinks;
	std::vector<UnknownField> Unknowns;


//protected:
public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	Contact();
	~Contact();

	uint64_t GetID() const { return m_recordId; }
	std::string GetPostalAddress() const;

	void Parse(const Data &data, unsigned int operation);
	void Clear();			// erase everything

	void Dump(std::ostream &os) const;
	void DumpLdif(std::ostream &os, const std::string &baseDN) const;
};

inline std::ostream& operator<< (std::ostream &os, const Contact &contact) {
	contact.Dump(os);
	return os;
}

class Message
{
public:
	// protocol record types, for size calculations
	typedef Barry::OldMessageRecord		OldProtocolRecordType;
	typedef Barry::MessageRecord		ProtocolRecordType;

	struct Address
	{
		std::string Name;
		std::string Email;
	};


	Address From;
	Address To;
	Address Cc;
	std::string Subject;
	std::string Body;
	std::vector<UnknownField> Unknowns;

public:
	Message();
	~Message();

	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
	void Parse(const Data &data, unsigned int operation);
	void Clear();

	void Dump(std::ostream &os) const;
};

inline std::ostream& operator<<(std::ostream &os, const Message &msg) {
	msg.Dump(os);
	return os;
}

std::ostream& operator<<(std::ostream &os, const Message::Address &msga);


class Calendar
{
public:
	// protocol record types, for size calculations
	typedef Barry::OldCalendarRecord	OldProtocolRecordType;
	typedef Barry::CalendarRecord		ProtocolRecordType;

private:
	uint64_t m_recordId;

public:
	std::string Subject;
	std::string Notes;
	std::string Location;
	time_t NotificationTime;
	time_t StartTime;
	time_t EndTime;
	std::vector<UnknownField> Unknowns;

public:
	Calendar();
	~Calendar();

	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
	void Parse(const Data &data, unsigned int operation);
	void Clear();

	void Dump(std::ostream &os) const;
};

inline std::ostream& operator<<(std::ostream &os, const Calendar &msg) {
	msg.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

