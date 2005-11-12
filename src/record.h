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

std::ostream& operator<< (std::ostream &os, const Contact &contact) {
	contact.Dump(os);
	return os;
}


} // namespace Syncberry

#endif

