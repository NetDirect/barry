///
/// \file	record.cc
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

#include "record.h"

namespace Syncberry {

// Contact field format
struct ContactField
{
	uint16_t	size;		// including null terminator
	uint8_t		type;
	uint8_t		data[1];
} __attribute__ ((packed));

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
#define CRC_NOTES		64
#define CRC_INVALID_FIELD	255

// Contact code to field table
struct ContactFieldLink
{
	int code;
	std::string Contact::* field;
};

ContactFieldLink ContactFieldLinks[] = {
	{ CRC_EMAIL,			&Contact::Email },
	{ CRC_PHONE,			&Contact::Phone },
	{ CRC_FAX,			&Contact::Fax },
	{ CRC_WORK_PHONE,		&Contact::WorkPhone },
	{ CRC_HOME_PHONE,		&Contact::HomePhone },
	{ CRC_MOBILE_PHONE,		&Contact::MobilePhone },
	{ CRC_PAGER,			&Contact::Pager },
	{ CRC_PIN,			&Contact::PIN },
	{ CRC_NAME,			&Contact::Name },
	{ CRC_NAME,			&Contact::LastName },
	{ CRC_COMPANY,			&Contact::Company },
	{ CRC_DEFAULT_COMM_METHOD,	&Contact::DefaultCommunicationsMethod },
	{ CRC_ADDRESS1,			&Contact::Address1 },
	{ CRC_ADDRESS2,			&Contact::Address2 },
	{ CRC_ADDRESS3,			&Contact::Address3 },
	{ CRC_CITY,			&Contact::City },
	{ CRC_PROVINCE,			&Contact::Province },
	{ CRC_POSTAL_CODE,		&Contact::PostalCode },
	{ CRC_COUNTRY,			&Contact::Country },
	{ CRC_TITLE,			&Contact::Title },
	{ CRC_PUBLIC_KEY,		&Contact::PublicKey },
	{ CRC_NOTES,			&Contact::Notes },
	{ CRC_INVALID_FIELD,		0 }
};


} // namespace Syncberry

