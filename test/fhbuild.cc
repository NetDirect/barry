///
/// \file	fhbuild.cc
///		Compile-time test to make sure all FieldHandle<> templates
///		and classes are complete.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <barry/barry.h>
#include <iostream>

using namespace std;
using namespace Barry;

void DumpId(const FieldIdentity &id)
{
	cout << "   sub = " << (id.HasSubfields ? "true" : "false")
		<< ", parent = "
		<< (id.ParentName ? id.ParentName : "NULL") << endl;
	cout << "   FieldTypeCode = " << id.FieldTypeCode << endl;
	cout << "   Ldif = " << (id.Ldif ? id.Ldif : "NULL")
		<< ", ObjectClass = "
		<< (id.ObjectClass ? id.ObjectClass : "NULL")
		<< endl;
	cout << "   Iconv = " << (id.IconvNeeded ? "true" : "false")
		<< endl << endl;
}

void DumpEnumConstants(const EnumConstants *ec)
{
	cout << "   Available constants:" << endl;

	EnumConstants::EnumConstantList::const_iterator
		b = ec->GetConstantList().begin(),
		e = ec->GetConstantList().end();
	for( ; b != e; ++b ) {
		cout << "      " << b->DisplayName
			<< " (" << b->Name << ")"
			<< " = " << b->Value << endl;
	}
	cout << endl;
}

std::ostream& operator<<(std::ostream &os, const EmailList &el)
{
	bool first = true;
	for( EmailList::const_iterator b = el.begin(); b != el.end(); ++b ) {
		if( !first )
			os << ", ";
		else
			first = false;

		os << *b;
	}
	return os;
}

template <class RecordT>
struct FieldHandler
{
	const RecordT &m_rec;

	FieldHandler(const RecordT &obj)
		: m_rec(obj)
	{
	}

	void operator()(EnumFieldBase<RecordT> *ep,
		const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< ep->GetName(ep->GetValue(m_rec))
			<< " (" << ep->GetValue(m_rec) << ")"
			<< endl;
		DumpId(id);
		DumpEnumConstants(ep);
	}

	void operator()(typename FieldHandle<RecordT>::PostalPointer pp,
		const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< m_rec.*(pp.m_PostalAddress).*(pp.m_PostalField)
			<< endl;
		DumpId(id);
	}

	template <class TypeT>
	void operator()(TypeT RecordT::* mp, const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< m_rec.*mp << endl;
		DumpId(id);
	}
};

class VirtualFieldHandler : public FieldValueHandlerBase
{
public:
	/// For type std::string
	virtual void operator()(const std::string &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type EmailAddressList
	virtual void operator()(const EmailAddressList &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type time_t
	virtual void operator()(const time_t &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type uint8_t
	virtual void operator()(const uint8_t &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< (unsigned int) v << endl;
		DumpId(id);
	}
	/// For type uint16_t
	virtual void operator()(const uint16_t &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type uint32_t
	virtual void operator()(const uint32_t &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type uint64_t
	virtual void operator()(const uint64_t &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type bool
	virtual void operator()(const bool &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type int32_t
	virtual void operator()(const int32_t &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type EmailList
	virtual void operator()(const EmailList &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type Date
	virtual void operator()(const Date &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type CategoryList
	virtual void operator()(const CategoryList &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type PostalAddress
	virtual void operator()(const PostalAddress &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
	/// For type UnknownsType
	virtual void operator()(const UnknownsType &v,
				const FieldIdentity &id) const
	{
		cout << id.DisplayName << " (" << id.Name << "): "
			<< v << endl;
		DumpId(id);
	}
};

void TestContact()
{
	Contact contact;
	contact.RecType = 8;
	contact.RecordId = 100000;
	contact.EmailAddresses.push_back("cdfrey@foursquare.net");
	contact.EmailAddresses.push_back("cdfrey@netdirect.ca");
	contact.Phone = "519-555-1212";
	contact.Fax = "no fax";
	contact.HomeFax = "home fax";
	contact.WorkPhone = "work 51235";
	contact.WorkPhone2 = "work2 12351234";
	contact.HomePhone = "home 12341234";
	contact.HomePhone2 = "home2 4234234";
	contact.MobilePhone = "mobile 88888";
	contact.MobilePhone2 = "mobile2 23424";
	contact.Pager = "pager 123412342";
	contact.PIN = "PIN 1234";
	contact.Radio = "CBC";
	contact.OtherPhone = "other 12341234";
	contact.FirstName = "Chris";
	contact.LastName = "Frey";
	contact.Company = "Company naaaaaaaaaaame";
	contact.DefaultCommunicationsMethod = "E";
	contact.JobTitle = "Programmer";
	contact.PublicKey = "seeeecrit";
	contact.URL = "http://netdirect.ca/~cdfrey/";
	contact.Prefix = "nothing";
	contact.Notes = "This is a notes field that can hold mega data";
	contact.UserDefined1 = "user 1";
	contact.UserDefined2 = "user 2";
	contact.UserDefined3 = "user 3";
	contact.UserDefined4 = "user 4";
	contact.Image = "no image available";
	contact.Nickname = "LRU";
	Date d;
	d.FromYYYYMMDD("20120101");
	contact.Birthday = d;
	contact.WorkAddress.Address1 = "work address 1";
	contact.WorkAddress.Address2 = "work address 2";
	contact.WorkAddress.Address3 = "work address 3";
	contact.WorkAddress.City = "work city";
	contact.WorkAddress.Province = "work Province";
	contact.WorkAddress.PostalCode = "work PostalCode";
	contact.WorkAddress.Country = "work Country";
	contact.HomeAddress.Address1 = "home address 1";
	contact.HomeAddress.Address2 = "home address 2";
	contact.HomeAddress.Address3 = "home address 3";
	contact.HomeAddress.City = "home city";
	contact.HomeAddress.Province = "home Province";
	contact.HomeAddress.PostalCode = "home PostalCode";
	contact.HomeAddress.Country = "home Country";
	contact.Categories.push_back("office");
	contact.Categories.push_back("entertainment");

	FieldHandler<Contact> handler(contact);
	ForEachField(Contact::GetFieldHandles(), handler);

	cout << "================================================" << endl;

	VirtualFieldHandler vhandler;
	ForEachFieldValue(contact, vhandler);
}

void BlankTestAll()
{
	VirtualFieldHandler vhandler;

	cout << "All parsers......" << endl;

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	cout << "Record: " << #tname << endl; \
	cout << "===================================================" << endl;\
	tname obj##tname; \
	FieldHandler<tname> fh##tname(obj##tname); \
	cout << "By member pointer..." << endl; \
	ForEachField(tname::GetFieldHandles(), fh##tname); \
	cout << "\nBy value..." << endl; \
	ForEachFieldValue(obj##tname, vhandler); \
	cout << endl;

	ALL_KNOWN_PARSER_TYPES


	cout << "All builders........" << endl;

#undef HANDLE_BUILDER
#define HANDLE_BUILDER(tname) \
	cout << "Record: " << #tname << endl; \
	cout << "===================================================" << endl;\
	tname bobj##tname; \
	FieldHandler<tname> bfh##tname(bobj##tname); \
	cout << "By member pointer..." << endl; \
	ForEachField(tname::GetFieldHandles(), bfh##tname); \
	cout << "\nBy value..." << endl; \
	ForEachFieldValue(bobj##tname, vhandler); \
	cout << endl;

	ALL_KNOWN_BUILDER_TYPES
}

int main()
{
	TestContact();
	BlankTestAll();
}

