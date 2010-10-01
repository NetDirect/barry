///
/// \file	vcard.h
///		Conversion routines for vcards
///

/*
    Copyright (C) 2006-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_VCARD_H__
#define __BARRY_SYNC_VCARD_H__

#include "dll.h"
#include "vbase.h"
#include "r_contact.h"
#include <stdint.h>
#include <string>

namespace Barry { namespace Sync {

//
// vCard
//
/// Class for converting between RFC 2425/2426 vCard data format,
/// and the Barry::Contact class.
///
class BXEXPORT vCard : public vBase
{
	// data to pass to external requests
	char *m_gCardData;	// dynamic memory returned by vformat()... can
				// be used directly by the plugin, without
				// overmuch allocation and freeing (see Extract())
	std::string m_vCardData;// copy of m_gCardData, for C++ use
	Barry::Contact m_BarryContact;

protected:
	void AddAddress(const char *rfc_type, const Barry::PostalAddress &addr);
	void AddCategories(const Barry::CategoryList &categories);
	void AddPhoneCond(const std::string &phone);
	void AddPhoneCond(const char *rfc_type, const std::string &phone);

	void ParseAddress(vAttr &adr, Barry::PostalAddress &address);
	void ParseCategories(vAttr &cat, Barry::CategoryList &cats);

public:
	vCard();
	~vCard();

	const std::string&	ToVCard(const Barry::Contact &con);
	const Barry::Contact&	ToBarry(const char *vcal, uint32_t RecordId);

	const std::string&	GetVCard() const { return m_vCardData; }
	const Barry::Contact&	GetBarryContact() const { return m_BarryContact; }

	char* ExtractVCard();

	void Clear();
};

}} // namespace Barry::Sync

#endif

