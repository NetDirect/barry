///
/// \file	r_contact.h
///		Blackberry database record parser class for contact records.
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_CONTACT_H__
#define __BARRY_RECORD_CONTACT_H__

#include "dll.h"
#include "record.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//

struct BXEXPORT ContactGroupLink
{
	uint32_t Link;
	uint16_t Unknown;

	ContactGroupLink() : Link(0), Unknown(0) {}
	ContactGroupLink(uint32_t link, uint16_t unknown)
		: Link(link), Unknown(unknown)
	{}
};

/// \addtogroup RecordParserClasses
/// @{

//
// Contact record class
//
/// Represents a single record in the Address Book Blackberry database.
///
class BXEXPORT Contact
{
public:
	typedef Barry::CategoryList			CategoryList;
	typedef ContactGroupLink			GroupLink;
	typedef std::vector<GroupLink>			GroupLinksType;
	typedef Barry::UnknownsType			UnknownsType;
	typedef std::string				EmailType;
	typedef std::vector<EmailType>			EmailList;

	//
	// Record fields
	//

	// contact specific data
	uint8_t RecType;
	uint32_t RecordId;
	EmailList EmailAddresses;

	/// This field, Phone, is deprecated.  It is possible
	/// to write to this field to the Blackberry,
	/// but modern devices won't let you add it
	/// through their GUIs.  This field only seems
	/// to exist on the 7750.  While other devices
	/// accept the field and display it, it is
	/// not accessible by default.
	std::string Phone;

	std::string
		Fax,
		WorkPhone,
		HomePhone,
		MobilePhone,
		Pager,
		PIN,
		Radio,
		WorkPhone2,
		HomePhone2,
		OtherPhone,
		FirstName,
		LastName,
		Company,
		DefaultCommunicationsMethod,
		JobTitle,
		PublicKey,
		URL,
		Prefix,
		Notes,
		UserDefined1,
		UserDefined2,
		UserDefined3,
		UserDefined4,
		Image;

	Date Birthday;
	Date Anniversary;

	PostalAddress WorkAddress;
	PostalAddress HomeAddress;

	// Categories are not allowed to have commas in them.
	// A category name containing a comma will be split into
	// two categories, not only by this library, but by the
	// device itself.
	CategoryList Categories;

	GroupLinksType GroupLinks;
	UnknownsType Unknowns;

private:
	bool m_FirstNameSeen;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	Contact();
	~Contact();

	uint32_t GetID() const { return RecordId; }
	std::string GetFullName() const;
	const std::string& GetEmail(unsigned int index = 0) const;

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	void Clear();			// erase everything

	void Dump(std::ostream &os) const;

	// Sorting - use enough data to make the sorting as
	//           consistent as possible
	bool operator<(const Contact &other) const;

	// database name
	static const char * GetDBName() { return "Address Book"; }
	static uint8_t GetDefaultRecType() { return 0; }

	// helpers
	static void SplitName(const std::string &full, std::string &first, std::string &last);
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const Contact &contact) {
	contact.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

