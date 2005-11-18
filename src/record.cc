///
/// \file	record.cc
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

#include "record.h"
#include "data.h"
#include <ostream>
#include <iomanip>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;

namespace Syncberry {

struct GroupLink
{
	uint64_t	uniqueId : 48;
} __attribute__ ((packed));

// Contact field format
struct ContactField
{
	uint16_t	size;		// including null terminator
	uint8_t		type;

	union FieldData
	{
		GroupLink	link;
		uint8_t		raw[1];
	} __attribute__ ((packed)) data;
} __attribute__ ((packed));

struct ContactRecord
{
	uint64_t	uniqueId : 48;	// only 48 bits of this is used in the
					// link above, so assuming that's all
					// there is
	uint8_t	unknown;
	ContactField	field[1];
} __attribute__ ((packed));

#define CONTACT_RECORD_HEADER_SIZE	(sizeof(Syncberry::ContactRecord) - sizeof(Syncberry::ContactField))
#define CONTACT_FIELD_HEADER_SIZE	(sizeof(Syncberry::ContactField) - sizeof(Syncberry::ContactField::FieldData))


// Contact record codes
#define CRC_EMAIL		1
#define CRC_PHONE		2
#define CRC_FAX			3
#define CRC_WORK_PHONE		6
#define CRC_HOME_PHONE		7
#define CRC_MOBILE_PHONE	8
#define CRC_PAGER		9
#define CRC_PIN			10
#define CRC_NAME		32	// used twice, in first/last name order
#define CRC_COMPANY		33
#define CRC_DEFAULT_COMM_METHOD	34
#define CRC_ADDRESS1		35
#define CRC_ADDRESS2		36
#define CRC_ADDRESS3		37
#define CRC_CITY		38
#define CRC_PROVINCE		39
#define CRC_POSTAL_CODE		40
#define CRC_COUNTRY		41
#define CRC_TITLE		42
#define CRC_PUBLIC_KEY		43
#define CRC_GROUP_LINK		52
#define CRC_NOTES		64
#define CRC_INVALID_FIELD	255

// Contact code to field table
struct ContactFieldLink
{
	int type;
	char *name;
	std::string Contact::* member;
};

ContactFieldLink ContactFieldLinks[] = {
	{ CRC_EMAIL,		"Email",	&Contact::Email },
	{ CRC_PHONE,		"Phone",	&Contact::Phone },
	{ CRC_FAX,		"Fax",		&Contact::Fax },
	{ CRC_WORK_PHONE,	"WorkPhone",	&Contact::WorkPhone },
	{ CRC_HOME_PHONE,	"HomePhone",	&Contact::HomePhone },
	{ CRC_MOBILE_PHONE,	"MobilePhone",	&Contact::MobilePhone },
	{ CRC_PAGER,		"Pager",	&Contact::Pager },
	{ CRC_PIN,		"PIN",		&Contact::PIN },
	{ CRC_COMPANY,		"Company",	&Contact::Company },
	{ CRC_DEFAULT_COMM_METHOD,"DefaultCommMethod",&Contact::DefaultCommunicationsMethod },
	{ CRC_ADDRESS1,		"Address1",	&Contact::Address1 },
	{ CRC_ADDRESS2,		"Address2",	&Contact::Address2 },
	{ CRC_ADDRESS3,		"Address3",	&Contact::Address3 },
	{ CRC_CITY,		"City",		&Contact::City },
	{ CRC_PROVINCE,		"Province",	&Contact::Province },
	{ CRC_POSTAL_CODE,	"PostalCode",	&Contact::PostalCode },
	{ CRC_COUNTRY,		"Country",	&Contact::Country },
	{ CRC_TITLE,		"Title",	&Contact::Title },
	{ CRC_PUBLIC_KEY,	"PublicKey",	&Contact::PublicKey },
	{ CRC_NOTES,		"Notes",	&Contact::Notes },
	{ CRC_INVALID_FIELD,	"EndOfList",	0 }
};

Contact::Contact()
{
}

Contact::~Contact()
{
}

void Contact::Parse(const unsigned char *begin, const unsigned char *end)
{
	// check size
	unsigned int totalSize = end - begin;
	if( totalSize < CONTACT_RECORD_HEADER_SIZE ) {
		dout("Contact: not enough data for parsing");
		return;			// nothing to do
	}

	const ContactRecord *record = (const ContactRecord *) begin;
	m_recordId = record->uniqueId;

	// advance pointer and cycle through all fields
	begin += CONTACT_RECORD_HEADER_SIZE;
	while( (begin + CONTACT_FIELD_HEADER_SIZE) < end )
		begin = ParseField(begin, end);
}

const unsigned char* Contact::ParseField(const unsigned char *begin,
					 const unsigned char *end)
{
	const ContactField *field = (const ContactField *) begin;

	// advance and check size
	begin += CONTACT_FIELD_HEADER_SIZE + field->size;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	ContactFieldLink *b = ContactFieldLinks;
		b->type != CRC_INVALID_FIELD;
		b++ )
	{
		if( b->type == field->type ) {
			std::string &s = this->*(b->member);
			s.assign((const char *)field->data.raw, field->size-1);
			return begin;	// done!
		}
	}

	// if not found in the type table, check for special handling
	switch( field->type )
	{
	case CRC_NAME: {
		// can be used multiple times, for first/last names
		std::string *name;
		if( FirstName.size() )
			// first name already filled, use last name
			name = &LastName;
		else
			name = &FirstName;

		name->assign((const char*)field->data.raw, field->size-1);
		}
		return begin;

	case CRC_GROUP_LINK:
		// just add the unique ID to the list
		GroupLinks.push_back(field->data.link.uniqueId);
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->data.raw, field->size);
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void Contact::Parse(const Data &data, int offset)
{
	if( offset < data.GetSize() )
		Parse(data.GetData() + offset, data.GetData() + data.GetSize());
}

void Contact::Dump(std::ostream &os) const
{
	ios::fmtflags oldflags = os.setf(ios::left);
	char fill = os.fill(' ');

	os << "Contact: 0x" << setbase(16) << GetID() << "\n";

	// special fields not in type table
	os << "    " << setw(20) << "FirstName";
	os << ": " << FirstName << "\n";
	os << "    " << setw(20) << "LastName";
	os << ": " << LastName << "\n";

	// cycle through the type table
	for(	ContactFieldLink *b = ContactFieldLinks;
		b->type != CRC_INVALID_FIELD;
		b++ )
	{
		// print only fields with data
		const std::string &field = this->*(b->member);
		if( field.size() ) {
			os << "    " << setw(20) << b->name;
			os << ": " << field << "\n";
		}
	}

	// print any group links
	std::vector<uint64_t>::const_iterator
		gb = GroupLinks.begin(), ge = GroupLinks.end();
	if( gb != ge )
		os << "    GroupLinks:\n";
	for( ; gb != ge; gb++ ) {
		os << "        ID: 0x" << setbase(16) << *gb << "\n";
	}

	// and finally print unknowns
	std::vector<UnknownField>::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	if( ub != ue )
		os << "    Unknowns:\n";
	for( ; ub != ue; ub++ ) {
		os << "        Type: 0x" << setbase(16)
		   << (unsigned int) ub->type
		   << " Data:\n" << Data(ub->data.data(), ub->data.size());
	}

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
}



///////////////////////////////////////////////////////////////////////////////
// CommandTable class

// CommandTable field format
struct CommandField
{
	uint8_t		size;		// no null terminator
	uint8_t		code;
	uint8_t		name[1];
} __attribute__ ((packed));

#define COMMAND_FIELD_HEADER_SIZE	(sizeof(::Syncberry::CommandField) - 1)

CommandTable::CommandTable()
{
}

CommandTable::~CommandTable()
{
}

void CommandTable::Parse(const unsigned char *begin, const unsigned char *end)
{
	while( begin < end )
		begin = ParseField(begin, end);
}

const unsigned char* CommandTable::ParseField(const unsigned char *begin,
					      const unsigned char *end)
{
	const CommandField *field = (const CommandField *) begin;

	// advance and check size
	begin += COMMAND_FIELD_HEADER_SIZE + field->size;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	Command command;
	command.Code = field->code;
	command.Name.assign((const char *)field->name, field->size);
	Commands.push_back(command);
	return begin;
}

void CommandTable::Parse(const Data &data, int offset)
{
	if( offset < data.GetSize() )
		Parse(data.GetData() + offset, data.GetData() + data.GetSize());
}

void CommandTable::Clear()
{
	Commands.clear();
}

unsigned int CommandTable::GetCommand(const std::string &name) const
{
	CommandArrayType::const_iterator b = Commands.begin();
	for( ; b != Commands.end(); b++ )
		if( b->Name == name )
			return b->Code;
	return 0;
}

void CommandTable::Dump(std::ostream &os) const
{
	CommandArrayType::const_iterator b = Commands.begin();
	os << "Command table:\n";
	for( ; b != Commands.end(); b++ ) {
		os << "    Command: 0x" << setbase(16) << b->Code
		   << " '" << b->Name << "'\n";
	}
}




///////////////////////////////////////////////////////////////////////////////
// DatabaseDatabase class

struct DBDBField
{
	uint16_t	dbNumber;
	uint8_t		unknown1;
	uint32_t	dbSize;			// assumed from Cassis docs...
						// always 0 in USB
	uint32_t	dbRecordCount;
	uint16_t	unknown2;
	uint16_t	nameSize;		// includes null terminator
	uint8_t		unknown3;
	uint8_t		name[1];		// followed by 2 zeros!
} __attribute__ ((packed));

#define DBDB_FIELD_HEADER_SIZE	(sizeof(::Syncberry::DBDBField) - 1)

DatabaseDatabase::DatabaseDatabase()
{
}

DatabaseDatabase::~DatabaseDatabase()
{
}

void DatabaseDatabase::Parse(const unsigned char *begin,
			     const unsigned char *end)
{
	while( begin < end )
		begin = ParseField(begin, end);
}

const unsigned char* DatabaseDatabase::ParseField(const unsigned char *begin,
						  const unsigned char *end)
{
	const DBDBField *field = (const DBDBField *) begin;

	// advance and check size
	begin += DBDB_FIELD_HEADER_SIZE + field->nameSize + 2;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->nameSize )		// if field has no size, something's up
		return begin;

	Database db;
	db.Number = field->dbNumber;
	db.RecordCount = field->dbRecordCount;
	db.Name.assign((const char *)field->name, field->nameSize - 1);
	Databases.push_back(db);
	return begin;
}

void DatabaseDatabase::Parse(const Data &data, int offset)
{
	if( offset < data.GetSize() )
		Parse(data.GetData() + offset, data.GetData() + data.GetSize());
}

void DatabaseDatabase::Clear()
{
	Databases.clear();
}

unsigned int DatabaseDatabase::GetDBNumber(const std::string &name) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	for( ; b != Databases.end(); b++ )
		if( b->Name == name )
			return b->Number;
	return 0;
}

void DatabaseDatabase::Dump(std::ostream &os) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	os << "Database database:\n";
	for( ; b != Databases.end(); b++ ) {
		os << "    Database: 0x" << setbase(16) << b->Number
		   << " '" << b->Name << "' (records: "
		   << setbase(10) << b->RecordCount << ")\n";
	}
}



} // namespace Syncberry


#ifdef __TEST_MODE__

#include <iostream>

int main(int argc, char *argv[])
{
	if( argc < 2 ) {
		cerr << "Usage: test <datafile>" << endl;
		return 1;
	}

	std::vector<Data> array;
	if( !LoadDataArray(argv[1], array) ) {
		cerr << "Unable to load file: " << argv[1] << endl;
		return 1;
	}

	for( std::vector<Data>::iterator b = array.begin(), e = array.end();
		b != e; b++ )
	{
		Data &d = *b;
//		cout << d << endl;
		if( d.GetSize() > 13 && d.GetData()[6] == 0x4f ) {
			Syncberry::Contact contact;
			contact.Parse(d, 13);
			cout << contact << endl;
		}
	}
}

#endif

