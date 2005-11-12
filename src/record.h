///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

#ifndef __SYNCBERRY_RECORD_H__
#define __SYNCBERRY_RECORD_H__

#include <iosfwd>

namespace Syncberry {

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//

class Contact
{
	// private contact management data

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


public:
	Contact();
	~Contact();
};

std::ostream& operator<< (std::ostream &os, const Contact &contact);


} // namespace Syncberry

#endif

