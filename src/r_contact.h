///
/// \file	r_contact.h
///		Blackberry database record parser class for contact records.
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "record.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

namespace Barry {

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//

struct ContactGroupLink
{
	uint32_t Link;
	uint16_t Unknown;

	ContactGroupLink() : Link(0), Unknown(0) {}
	ContactGroupLink(uint32_t link, uint16_t unknown)
		: Link(link), Unknown(unknown)
	{}
};

typedef std::vector<std::string> CategoryList;

/// \addtogroup RecordParserClasses
/// @{

class Contact
{
public:
	typedef Barry::CategoryList			CategoryList;
	typedef ContactGroupLink			GroupLink;
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

//protected:
public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	Contact();
	~Contact();

	uint32_t GetID() const { return RecordId; }
	std::string GetFullName() const;

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
	static void CategoryStr2List(const std::string &str, Barry::CategoryList &list);
	static void CategoryList2Str(const Barry::CategoryList &list, std::string &str);
};

inline std::ostream& operator<< (std::ostream &os, const Contact &contact) {
	contact.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

