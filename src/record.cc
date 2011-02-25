///
/// \file	record.cc
///		Misc. Blackberry database record helper classes and functions.
///		Helps translate data from data packets to useful structures,
///		and back.
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "record.h"
#include "record-internal.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "error.h"
#include "endian.h"
#include "trim.h"
#include <sstream>
#include <iomanip>
#include <time.h>
#include <string.h>
#include <stdio.h>			// for sscanf()
#include <stdexcept>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

BXEXPORT std::ostream& operator<< (std::ostream &os, const Cr2LfWrapper &str)
{
	for(	std::string::const_iterator i = str.m_str.begin();
		i != str.m_str.end() && *i;
		i++)
	{
		if( *i == '\r' )
			os << '\n';
		else
			os << *i;
	}
	return os;
}

//////////////////////////////////////////////////////////////////////////////
// Field builder helper functions

void BuildField1900(Data &data, size_t &size, uint8_t type, time_t t)
{
	size_t timesize = COMMON_FIELD_MIN1900_SIZE;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + timesize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(timesize);
	field->type = type;
	field->u.min1900 = time2min(t);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, char c)
{
	BuildField(data, size, type, (uint8_t)c);
}

void BuildField(Data &data, size_t &size, uint8_t type, uint8_t c)
{
	size_t strsize = 1;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(strsize);
	field->type = type;
	memcpy(field->u.raw, &c, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, uint16_t value)
{
	size_t strsize = 2;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(strsize);
	field->type = type;

	uint16_t store = htobs(value);
	memcpy(field->u.raw, &store, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, uint32_t value)
{
	size_t strsize = 4;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobl(strsize);
	field->type = type;

	uint32_t store = htobl(value);
	memcpy(field->u.raw, &store, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, uint64_t value)
{
	size_t strsize = 8;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobl(strsize);
	field->type = type;

	uint64_t store = htobll(value);
	memcpy(field->u.raw, &store, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, const std::string &str)
{
	// include null terminator
	BuildField(data, size, type, str.c_str(), str.size() + 1);
}

void BuildField(Data &data, size_t &size, uint8_t type,
		const void *buf, size_t bufsize)
{
	// include null terminator
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + bufsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(bufsize);
	field->type = type;
	memcpy(field->u.raw, buf, bufsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, const Barry::UnknownField &field)
{
	BuildField(data, size, field.type,
		field.data.raw_data.data(), field.data.raw_data.size());
}

void BuildField(Data &data, size_t &size, uint8_t type, const Barry::Protocol::GroupLink &link)
{
	size_t linksize = sizeof(Barry::Protocol::GroupLink);
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + linksize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(linksize);
	field->type = type;
	field->u.link = link;

	size += fieldsize;
}

std::string ParseFieldString(const Barry::Protocol::CommonField *field)
{
	// make no assumptions here, and pass the full size in as
	// the maxlen, even though 99% of the time, it will be a null...
	// this function can be used by non-null terminated strings as well
	return ParseFieldString(field->u.raw, btohs(field->size));
}

std::string ParseFieldString(const void *data, uint16_t maxlen)
{
	const char *str = (const char *)data;

	// find last non-null character, since some fields
	// can have multiple null terminators
	while( maxlen && str[maxlen-1] == 0 )
		maxlen--;

	return std::string(str, maxlen);
}


///////////////////////////////////////////////////////////////////////////////
// UnknownField

std::ostream& operator<< (std::ostream &os, const std::vector<UnknownField> &unknowns)
{
	std::vector<UnknownField>::const_iterator
		ub = unknowns.begin(), ue = unknowns.end();
	if( ub != ue )
		os << "    Unknowns:\n";
	for( ; ub != ue; ub++ ) {
		os << "        Type: 0x" << setbase(16)
		   << (unsigned int) ub->type
		   << " Data:\n" << Data(ub->data.data(), ub->data.size());
	}
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// EmailAddress class

EmailAddress::EmailAddress(const std::string &complex_address)
{
	size_t end = complex_address.rfind('>');
	size_t start = complex_address.rfind('<');
	if( start == string::npos || end == string::npos || start > end ) {
		// simple address, add it
		Email = complex_address;
		Inplace::trim(Email);
	}
	else {
		Name = complex_address.substr(0, start);
		Inplace::trim(Name);

		Email = complex_address.substr(start+1, end - start - 1);
		Inplace::trim(Email);
	}
}

std::ostream& operator<<(std::ostream &os, const EmailAddress &msga)
{
	if( msga.Name.size() )
		os << msga.Name << " <";
	os << msga.Email;
	if( msga.Name.size() )
		os << ">";
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// EmailAddressList class

std::string EmailAddressList::ToCommaSeparated() const
{
	std::ostringstream oss;
	oss << *this;
	return oss.str();
}

/// Adds every email address found in the comma separated list.
/// Does not clear() first.
void EmailAddressList::AddCommaSeparated(const std::string &list)
{
	istringstream iss(list);
	string address;
	iss >> ws;

	while( getline(iss, address, ',') ) {
		// trim any trailing whitespace in the address
		size_t len = address.size();
		while( len && ::isspace(address[len-1]) )
			address.resize(len-1);

		// add to list if anything left
		if( address.size() ) {
			EmailAddress ea(address);
			push_back(ea);
		}
	}
}

std::ostream& operator<<(std::ostream &os, const EmailAddressList &elist)
{
	for( EmailAddressList::const_iterator i = elist.begin(); i != elist.end(); ++i ) {
		if( i != elist.begin() )
			os << ", ";
		os << *i;
	}
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// PostalAddress class

//
// GetLabel
//
/// Format a mailing address into a single string, handling missing fields.
///
std::string PostalAddress::GetLabel() const
{
	std::string address = Address1;
	if( Address2.size() ) {
		if( address.size() )
			address += "\n";
		address += Address2;
	}
	if( Address3.size() ) {
		if( address.size() )
			address += "\n";
		address += Address3;
	}
	if( address.size() )
		address += "\n";
	if( City.size() )
		address += City + " ";
	if( Province.size() )
		address += Province + " ";
	if( Country.size() )
		address += Country;
	if( address.size() )
		address += "\n";
	if( PostalCode.size() )
		address += PostalCode;

	return address;
}

void PostalAddress::Clear()
{
	Address1.clear();
	Address2.clear();
	Address3.clear();
	City.clear();
	Province.clear();
	PostalCode.clear();
	Country.clear();
}

std::ostream& operator<<(std::ostream &os, const PostalAddress &post) {
	os << post.GetLabel();
	return os;
}



///////////////////////////////////////////////////////////////////////////////
// Date class

Date::Date(const struct tm *timep)
{
	FromTm(timep);
}

void Date::Clear()
{
	Month = Day = Year = 0;
}

void Date::ToTm(struct tm *timep) const
{
	memset(timep, 0, sizeof(tm));
	timep->tm_year = Year - 1900;
	timep->tm_mon = Month;
	timep->tm_mday = Day;
}

std::string Date::ToYYYYMMDD() const
{
	std::ostringstream oss;
	// setfill and setw not sticky.
	oss	<< setw(4) << setfill('0') << dec << Year
		<< setw(2) << setfill('0') << dec << (Month + 1)
		<< setw(2) << setfill('0') << dec << Day;
	return oss.str();
}

//
// ToBBString
//
/// The Blackberry stores Birthday and Anniversary date fields
/// with the format: DD/MM/YYYY
///
std::string Date::ToBBString() const
{
	std::ostringstream oss;
	// setw() ain't 'sticky'!
	oss	<< setw(2) << setfill('0') << dec << Day << '/'
		<< setw(2) << setfill('0') << dec << (Month + 1) << '/'
		<< setw(2) << setfill('0') << dec << Year;
	return oss.str();
}

bool Date::FromTm(const struct tm *timep)
{
	if( !timep )
		throw std::logic_error("NULL time pointer passed to Date::FromTm");

	Year = timep->tm_year + 1900;
	Month = timep->tm_mon;
	Day = timep->tm_mday;
	return true;
}

bool Date::FromBBString(const std::string &str)
{
	int m, d, y;
	if( 3 == sscanf(str.c_str(), "%d/%d/%d", &d, &m, &y) ) {
		Year = y;
		Month = m - 1;
		Day = d;
		return true;
	}
	return false;
}

bool Date::FromYYYYMMDD(const std::string &str)
{
	int m, d, y;
	if( 3 == sscanf(str.c_str(), "%4d%2d%2d", &y, &m, &d) ) {
		Year = y;
		Month = m - 1;
		Day = d;
		return true;
	}
	return false;
}

std::ostream& operator<<(std::ostream &os, const Date &date)
{
	os	<< setw(4) << dec << date.Year << '/'
		<< setw(2) << dec << (date.Month + 1) << '/'
		<< setw(2) << dec << date.Day;
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// CategoryList class

/// Parses the given comma delimited category string into
/// this CategoryList object, appending each token to the vector.
/// Will clear vector beforehand.
void CategoryList::CategoryStr2List(const std::string &str)
{
	// start fresh
	clear();

	if( !str.size() )
		return;

	// parse the comma-delimited string to a list, stripping away
	// any white space around each category name
	string::size_type start = 0, end = 0, delim = str.find(',', start);
	while( start != string::npos ) {
		if( delim == string::npos )
			end = str.size() - 1;
		else
			end = delim - 1;

		// strip surrounding whitespace
		while( str[start] == ' ' )
			start++;
		while( end && str[end] == ' ' )
			end--;

		if( start <= end ) {
			string token = str.substr(start, end-start+1);
			push_back(token);
		}

		// next
		start = delim;
		if( start != string::npos )
			start++;
		delim = str.find(',', start);
	}
}

/// Turns the current vectory into a comma delimited category
/// string suitable for use in Calendar, Task, and Memo protocol values.
void CategoryList::CategoryList2Str(std::string &str) const
{
	str.clear();

	Barry::CategoryList::const_iterator i = begin();
	for( ; i != end(); ++i ) {
		if( str.size() )
			str += ", ";
		str += *i;
	}
}


} // namespace Barry


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

	cout << "Loaded " << array.size() << " items" << endl;

	for( std::vector<Data>::iterator b = array.begin(), e = array.end();
		b != e; b++ )
	{
		Data &d = *b;
//		cout << d << endl;
		if( d.GetSize() > 13 && d.GetData()[6] == 0x4f ) {
			Barry::Contact contact;
			size_t size = 13;
			contact.ParseFields(d, size);
			cout << contact << endl;
			contact.DumpLdif(cout, "ou=People,dc=example,dc=com");
		}
		else if( d.GetSize() > 13 && d.GetData()[6] == 0x44 ) {
			Barry::Calendar cal;
			size_t size = 13;
			cal.ParseFields(d, size);
			cout << cal << endl;
		}
	}
}

#endif

