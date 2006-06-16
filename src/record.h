///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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
	struct GroupLink
	{
		uint32_t Link;
		uint16_t Unknown;

		GroupLink() : Link(0), Unknown(0) {}
		GroupLink(uint32_t link, uint16_t unknown)
			: Link(link), Unknown(unknown)
		{}
	};

	typedef std::vector<GroupLink>			GroupLinksType;
	typedef std::vector<UnknownField>		UnknownsType;

	// contact specific data
	uint32_t RecordId;
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

	GroupLinksType GroupLinks;
	UnknownsType Unknowns;


//protected:
public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	Contact();
	~Contact();

	uint64_t GetID() const { return RecordId; }
	std::string GetPostalAddress() const;

	void Parse(const Data &data, size_t offset, unsigned int operation);
	void Build(Data &data, size_t offset) const;
	void Clear();			// erase everything

	void Dump(std::ostream &os) const;
	void DumpLdif(std::ostream &os, const std::string &baseDN) const;

	// sorting - put group links at the end
	bool operator<(const Contact &other) const {
		return GroupLinks.size() == 0 && other.GroupLinks.size() > 0;
//		// testing - put group links at the top
//		return GroupLinks.size() > 0 && other.GroupLinks.size() == 0;
	}

	// protocol record sizes
	static size_t GetOldProtocolRecordSize();
	static size_t GetProtocolRecordSize();

	// database name
	static const char * GetDBName() { return "Address Book"; }
};

inline std::ostream& operator<< (std::ostream &os, const Contact &contact) {
	contact.Dump(os);
	return os;
}

class Message
{
public:
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
	void Parse(const Data &data, size_t offset, unsigned int operation);
	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const Message &other) const { return Subject < other.Subject; }

	// protocol record sizes
	static size_t GetOldProtocolRecordSize();
	static size_t GetProtocolRecordSize();

	// database name
	static const char * GetDBName() { return "Messages"; }
};

inline std::ostream& operator<<(std::ostream &os, const Message &msg) {
	msg.Dump(os);
	return os;
}

std::ostream& operator<<(std::ostream &os, const Message::Address &msga);


class Calendar
{
public:
	typedef std::vector<UnknownField>		UnknownsType;

	uint64_t RecordId;
	bool Recurring;
	bool AllDayEvent;
	std::string Subject;
	std::string Notes;
	std::string Location;
	time_t NotificationTime;
	time_t StartTime;
	time_t EndTime;
	UnknownsType Unknowns;

public:
	Calendar();
	~Calendar();

	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
	void Parse(const Data &data, size_t offset, unsigned int operation);
	void Build(Data &data, size_t offset) const;
	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const Calendar &other) const { return StartTime < other.StartTime; }

	// protocol record sizes
	static size_t GetOldProtocolRecordSize();
	static size_t GetProtocolRecordSize();

	// database name
	static const char * GetDBName() { return "Calendar"; }
};

inline std::ostream& operator<<(std::ostream &os, const Calendar &msg) {
	msg.Dump(os);
	return os;
}


// This is a packed field, which is a group of fields packed in
// variable length records inside one larger field of a normal record.
class ServiceBookConfig
{
public:
	typedef std::vector<UnknownField>		UnknownsType;

	uint8_t Format;

	UnknownsType Unknowns;

public:
	ServiceBookConfig();
	~ServiceBookConfig();

	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
	void Parse(const Data &data, size_t offset, unsigned int operation);
	void Build(Data &data, size_t offset) const;
	void Clear();

	void Dump(std::ostream &os) const;
};

inline std::ostream& operator<<(std::ostream &os, const ServiceBookConfig &msg) {
	msg.Dump(os);
	return os;
}


class ServiceBook
{
	int NameType, DescType, UniqueIdType;

public:
	typedef std::vector<UnknownField>		UnknownsType;

	uint64_t RecordId;
	std::string Name;
	std::string HiddenName;
	std::string Description;
	std::string DSID;
	std::string BesDomain;
	std::string UniqueId;
	std::string ContentId;
	ServiceBookConfig Config;

	UnknownsType Unknowns;

public:
	ServiceBook();
	~ServiceBook();

	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
	void Parse(const Data &data, size_t offset, unsigned int operation);
	void Build(Data &data, size_t offset) const;
	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const ServiceBook &other) const { return RecordId < RecordId; }

	// protocol record sizes
	static size_t GetOldProtocolRecordSize();
	static size_t GetProtocolRecordSize();

	// database name
	static const char * GetDBName() { return "Service Book"; }
};

inline std::ostream& operator<<(std::ostream &os, const ServiceBook &msg) {
	msg.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

