///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
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
	uint8_t RecType;
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

	uint32_t GetID() const { return RecordId; }
	std::string GetPostalAddress() const;

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset) const;

	void Clear();			// erase everything

	void Dump(std::ostream &os) const;

	// sorting - put group links at the end
	bool operator<(const Contact &other) const {
		return GroupLinks.size() == 0 && other.GroupLinks.size() > 0;
//		// testing - put group links at the top
//		return GroupLinks.size() > 0 && other.GroupLinks.size() == 0;
	}

	// database name
	static const char * GetDBName() { return "Address Book"; }
	static uint8_t GetDefaultRecType() { return 0; }

	// helpers
	static void SplitName(const std::string &full, std::string &first, std::string &last);
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
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	Message();
	~Message();

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const;
	uint32_t GetUniqueId() const;	// empty API, not required by protocol
	void SetIds(uint8_t Type, uint32_t Id);	// empty API, not required by protocol
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset) const;

	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const Message &other) const { return Subject < other.Subject; }

	// database name
	static const char * GetDBName() { return "Messages"; }
	static uint8_t GetDefaultRecType() { return 0; }
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

	uint8_t RecType;
	uint32_t RecordId;

	// general data
	bool AllDayEvent;
	std::string Subject;
	std::string Notes;
	std::string Location;
	time_t NotificationTime;
	time_t StartTime;
	time_t EndTime;

	///
	/// Recurring data
	///
	/// Note: interval can be used on all of these recurring types to
	///       make it happen "every other time" or more, etc.
	///
	enum RecurringCodeType {
		Day = 1,		//< eg. every day
					//< set: nothing
		MonthByDate = 3,	//< eg. every month on the 12th
					//< set: DayOfMonth
		MonthByDay = 4,		//< eg. every month on 3rd Wed
					//< set: DayOfWeek and WeekOfMonth
		YearByDate = 5,		//< eg. every year on March 5
					//< set: DayOfMonth and MonthOfYear
		YearByDay = 6,		//< eg. every year on 3rd Wed of Jan
					//< set: DayOfWeek, WeekOfMonth, and
					//<      MonthOfYear
		Week = 12		//< eg. every week on Mon and Fri
					//< set: WeekDays
	};

	bool Recurring;
	RecurringCodeType RecurringType;
	unsigned short Interval;	// must be >= 1
	time_t RecurringEndTime;	// only pertains if Recurring is true
					// sets the date and time when
					// recurrance of this appointment
					// should no longer occur
					// If a perpetual appointment, this
					// is 0xFFFFFFFF in the low level data
					// Instead, set the following flag.
	bool Perpetual;			// if true, this will always recur
	unsigned short TimeZoneCode;	// the time zone originally used
					// for the recurrance data...
					// seems to have little use, but
					// set to your current time zone
					// as a good default

	unsigned short			// recurring details, depending on type
		DayOfWeek,		// 0-6
		WeekOfMonth,		// 1-5
		DayOfMonth,		// 1-31
		MonthOfYear;		// 1-12
	unsigned char WeekDays;		// bitmask, bit 0 = sunday

// FIXME - put these somewhere usable by both C and C++
		#define CAL_WD_SUN	0x01
		#define CAL_WD_MON	0x02
		#define CAL_WD_TUE	0x04
		#define CAL_WD_WED	0x08
		#define CAL_WD_THU	0x10
		#define CAL_WD_FRI	0x20
		#define CAL_WD_SAT	0x40

	// unknown
	UnknownsType Unknowns;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
	void ParseRecurranceData(const void *data);
	void BuildRecurranceData(void *data);

public:
	Calendar();
	~Calendar();

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset) const;

	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const Calendar &other) const { return StartTime < other.StartTime; }

	// database name
	static const char * GetDBName() { return "Calendar"; }
	static uint8_t GetDefaultRecType() { return 5; }	// or 0?
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
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	ServiceBookConfig();
	~ServiceBookConfig();

	// Parser / Builder API (see parser.h / builder.h)
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset) const;

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

	uint8_t RecType;
	uint32_t RecordId;
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
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	ServiceBook();
	~ServiceBook();

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset) const;

	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const ServiceBook &other) const { return RecordId < RecordId; }

	// database name
	static const char * GetDBName() { return "Service Book"; }
	static uint8_t GetDefaultRecType() { return 0; }
};

inline std::ostream& operator<<(std::ostream &os, const ServiceBook &msg) {
	msg.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

