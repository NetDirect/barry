///
/// \file	s11n-boost.h
///		Non-intrusive versions of serialization functions for the
///		record classes.  These template functions make it possible
///		to use the record classes with the Boost::Serialization
///		library.
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_S11N_BOOST_H__
#define __BARRY_S11N_BOOST_H__

#include "record.h"
#include <boost/serialization/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
// special versions
//
// BARRY_BASE_S11N_VERSION      - the base version where all record data is
//                                stored so it can be fully retrieved and
//                                uploaded to the handheld device later.
// BARRY_POD_MAP_VERSION        - if these templates are called with a version
//                                equal or greater than this, only mappable,
//                                POD data is included in the serialization
//
#define BARRY_BASE_S11N_VERSION		0
#define BARRY_POD_MAP_VERSION		1000

// namespace boost::serialization, for the non-intrusive version
namespace boost {
namespace serialization {

template <class Archive>
void serialize(Archive &ar, Barry::UnknownField &uf, const unsigned int ver)
{
	ar & make_nvp("type", uf.type);
	ar & make_nvp("data", uf.data);
}

template <class Archive>
void serialize(Archive &ar, Barry::Contact::GroupLink &g, const unsigned int ver)
{
	ar & make_nvp("Link", g.Link);
	ar & make_nvp("Unknown", g.Unknown);
}

template <class Archive>
void serialize(Archive &ar, Barry::Contact &c, const unsigned int ver)
{
	ar & make_nvp("RecordId", c.RecordId);

	ar & make_nvp("Email", c.Email);
	ar & make_nvp("Phone", c.Phone);
	ar & make_nvp("Fax", c.Fax);
	ar & make_nvp("WorkPhone", c.WorkPhone);
	ar & make_nvp("HomePhone", c.HomePhone);
	ar & make_nvp("MobilePhone", c.MobilePhone);
	ar & make_nvp("Pager", c.Pager);
	ar & make_nvp("PIN", c.PIN);
	ar & make_nvp("FirstName", c.FirstName);
	ar & make_nvp("LastName", c.LastName);
	ar & make_nvp("Company", c.Company);
	ar & make_nvp("DefaultCommunicationsMethod", c.DefaultCommunicationsMethod);
	ar & make_nvp("Address1", c.Address1);
	ar & make_nvp("Address2", c.Address2);
	ar & make_nvp("Address3", c.Address3);
	ar & make_nvp("City", c.City);
	ar & make_nvp("Province", c.Province);
	ar & make_nvp("PostalCode", c.PostalCode);
	ar & make_nvp("Country", c.Country);
	ar & make_nvp("Title", c.Title);
	ar & make_nvp("PublicKey", c.PublicKey);
	ar & make_nvp("Notes", c.Notes);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("GroupLinks", c.GroupLinks);
		ar & make_nvp("Unknowns", c.Unknowns);
	}
}

template <class Archive>
void serialize(Archive &ar, Barry::Message::Address &a, const unsigned int ver)
{
	ar & make_nvp("Name", a.Name);
	ar & make_nvp("Email", a.Email);
}

template <class Archive>
void serialize(Archive &ar, Barry::Message &m, const unsigned int ver)
{
	ar & make_nvp("From", m.From);
	ar & make_nvp("To", m.To);
	ar & make_nvp("Cc", m.Cc);
	ar & make_nvp("Subject", m.Subject);
	ar & make_nvp("Body", m.Body);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", m.Unknowns);
	}
}

template <class Archive>
void serialize(Archive &ar, Barry::Calendar &c, const unsigned int ver)
{
	ar & make_nvp("RecordId", c.RecordId);

	ar & make_nvp("Subject", c.Subject);
	ar & make_nvp("Notes", c.Notes);
	ar & make_nvp("Location", c.Location);

	ar & make_nvp("NotificationTime", c.NotificationTime);
	ar & make_nvp("StartTime", c.StartTime);
	ar & make_nvp("EndTime", c.EndTime);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", c.Unknowns);
	}
}

}} // namespace boost::serialization

#endif

