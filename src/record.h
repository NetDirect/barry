///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

#ifndef __SYNCBERRY_RECORD_H__
#define __SYNCBERRY_RECORD_H__

#include <iosfwd>
#include <string>
#include <vector>
#include <stdint.h>

// forward declarations
class Data;

namespace Syncberry {

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//

class Contact
{
public:
	struct UnknownField
	{
		uint8_t type;
		std::string data;
	};

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


protected:
	void Parse(const unsigned char *begin, const unsigned char *end);
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	Contact();
	~Contact();

	uint64_t GetID() const { return m_recordId; }

	void Parse(const Data &data, int offset);
	void Clear();			// erase everything

	void Dump(std::ostream &os) const;
};

inline std::ostream& operator<< (std::ostream &os, const Contact &contact) {
	contact.Dump(os);
	return os;
}



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
	void Parse(const unsigned char *begin, const unsigned char *end);
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
	void Parse(const unsigned char *begin, const unsigned char *end);
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	DatabaseDatabase();
	~DatabaseDatabase();

	void Parse(const Data &data, int offset);
	void Clear();

	// FIXME - returns 0 on error here, but that's a valid DBNumber
	unsigned int GetDBNumber(const std::string &name) const;

	void Dump(std::ostream &os) const;
};

inline std::ostream& operator<<(std::ostream &os, const DatabaseDatabase &dbdb) {
	dbdb.Dump(os);
	return os;
}


} // namespace Syncberry

#endif

