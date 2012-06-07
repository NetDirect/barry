///
/// \file	ldif.h
///		Routines for reading and writing LDAP LDIF data.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_LDIF_H__
#define __BARRY_LDIF_H__

#include "dll.h"
#include <string>
#include <map>

// forward declarations
namespace Barry {
	class Contact;
}

namespace Barry {

//
// ContactLdif
//
/// Class for generating LDIF output based on a Barry::Contact record object.
/// This class supports LDIF attribute mapping, and a heuristics mechanism
/// for parsing LDIF fields that may not have consistent data.
///
/// To use this class, create an instance of it, then call DumpLdif(), passing
/// the Contact record object to base the work on.  Output will be written
/// to the stream you provide.  ReadLdif() goes in the other direction.
///
/// To override LDIF attribute mapping, call Map() or Unmap() as appropriate.
///
/// To get a list of supported Barry::Contact field names, call GetFieldNames().
/// This function returns a pointer to an array of ContactLdif::NameToFunc
/// structures, ending with NameToFunc::name as null.  You can cycle through the
/// array with code like this:
///
/// <pre>
///	for( ContactLdif::NameToFunc *n = o.GetFieldNames(); n->name; n++ ) {
///		...
///	}
/// </pre>
///
/// Note that all Get/Set functions used in attribute mapping are virtual,
/// and can be overridden by a derived class.  This includes the heuristics
/// functions, which are called by DumpLdif().
///
/// Note that the description field in NameToFunc is translatable.
/// You can pass it to gettext() for a translated string.
///
class BXEXPORT ContactLdif
{
public:
	typedef std::string (ContactLdif::*GetFunctionType)(const Barry::Contact&) const;
	typedef void (ContactLdif::*SetFunctionType)(Barry::Contact&, const std::string &) const;

	/// Used to create a List of supported Barry field names, including
	/// calculated names, such as full postal address.
	struct NameToFunc
	{
		const char *name;
		const char *description;
		GetFunctionType read;
		SetFunctionType write;
	};

	struct LdifAttribute
	{
		std::string name;
		std::string objectClass;
		int order;

		LdifAttribute() : order(0) {}
		LdifAttribute(const char *name, const std::string &oc = "")
			: name(name), objectClass(oc), order(0)
			{}
		LdifAttribute(const std::string &name, const std::string &oc = "")
			: name(name), objectClass(oc), order(0)
			{}

		bool operator<(const LdifAttribute &other) const;
		bool operator==(const LdifAttribute &other) const;
	};

	struct AccessPair
	{
		GetFunctionType read;
		SetFunctionType write;

		AccessPair() : read(0), write(0) {}
		AccessPair(GetFunctionType r, SetFunctionType w)
			: read(r), write(w)
			{}
	};

	typedef std::map<LdifAttribute, AccessPair>   AccessMapType;
	typedef std::map<std::string, std::string*>   HookMapType;

protected:
	static const NameToFunc FieldMap[];
	AccessMapType m_map;
	std::string m_baseDN;
	HookMapType m_hookMap;
	LdifAttribute m_dnAttr;

	void DoWrite(Barry::Contact &con, const std::string &attr,
		const std::string &data);

	// Array getter state
	mutable unsigned int m_emailIndex;

	// name heuristics
	std::string m_cn, m_displayName, m_sn, m_givenName;

	// heuristics hooking - saves each found value in the variable
	// pointed at by var
	void Hook(const std::string &ldifname, std::string *var);

public:
	explicit ContactLdif(const std::string &baseDN);
	virtual ~ContactLdif();

	const NameToFunc* GetFieldNames() const;
	const NameToFunc* GetField(const std::string &fieldname) const;
	std::string GetFieldReadName(GetFunctionType read) const;
	std::string GetFieldWriteName(SetFunctionType write) const;

	bool Map(const LdifAttribute &ldifname, const std::string &readField,
		const std::string &writeField);
	void Map(const LdifAttribute &ldifname, GetFunctionType read,
		SetFunctionType write);
	void Unmap(const LdifAttribute &ldifname);

	void SetBaseDN(const std::string &baseDN) { m_baseDN = baseDN; }
	bool SetDNAttr(const LdifAttribute &name);
	bool SetObjectClass(const LdifAttribute &name, const std::string &objectClass);
	bool SetObjectOrder(const LdifAttribute &name, int order);

	//
	// Access functions
	//

	virtual std::string Email(const Barry::Contact &con) const;
	virtual std::string Phone(const Barry::Contact &con) const;
	virtual std::string Fax(const Barry::Contact &con) const;
	virtual std::string WorkPhone(const Barry::Contact &con) const;
	virtual std::string HomePhone(const Barry::Contact &con) const;
	virtual std::string MobilePhone(const Barry::Contact &con) const;
	virtual std::string Pager(const Barry::Contact &con) const;
	virtual std::string PIN(const Barry::Contact &con) const;
	virtual std::string FirstName(const Barry::Contact &con) const;
	virtual std::string LastName(const Barry::Contact &con) const;
	virtual std::string Company(const Barry::Contact &con) const;
	virtual std::string DefaultCommunicationsMethod(const Barry::Contact &con) const;
	virtual std::string WorkAddress1(const Barry::Contact &con) const;
	virtual std::string WorkAddress2(const Barry::Contact &con) const;
	virtual std::string WorkAddress3(const Barry::Contact &con) const;
	virtual std::string WorkCity(const Barry::Contact &con) const;
	virtual std::string WorkProvince(const Barry::Contact &con) const;
	virtual std::string WorkPostalCode(const Barry::Contact &con) const;
	virtual std::string WorkCountry(const Barry::Contact &con) const;
	virtual std::string JobTitle(const Barry::Contact &con) const;
	virtual std::string PublicKey(const Barry::Contact &con) const;
	virtual std::string Notes(const Barry::Contact &con) const;
	virtual std::string Image(const Barry::Contact &con) const;
	// calculated values...
	virtual std::string WorkPostalAddress(const Barry::Contact &con) const;
	virtual std::string HomePostalAddress(const Barry::Contact &con) const;
	virtual std::string FullName(const Barry::Contact &con) const;
	virtual std::string FQDN(const Barry::Contact &con) const;

	//
	// Array modifier functions for above Access functions
	//

	virtual bool IsArrayFunc(GetFunctionType getf) const;
	void ClearArrayState() const;

	//
	// Write functions
	//

	virtual void SetEmail(Barry::Contact &con, const std::string &val) const;
	virtual void SetPhone(Barry::Contact &con, const std::string &val) const;
	virtual void SetFax(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkPhone(Barry::Contact &con, const std::string &val) const;
	virtual void SetHomePhone(Barry::Contact &con, const std::string &val) const;
	virtual void SetMobilePhone(Barry::Contact &con, const std::string &val) const;
	virtual void SetPager(Barry::Contact &con, const std::string &val) const;
	virtual void SetPIN(Barry::Contact &con, const std::string &val) const;
	virtual void SetFirstName(Barry::Contact &con, const std::string &val) const;
	virtual void SetLastName(Barry::Contact &con, const std::string &val) const;
	virtual void SetCompany(Barry::Contact &con, const std::string &val) const;
	virtual void SetDefaultCommunicationsMethod(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkAddress1(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkAddress2(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkAddress3(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkCity(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkProvince(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkPostalCode(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkCountry(Barry::Contact &con, const std::string &val) const;
	virtual void SetJobTitle(Barry::Contact &con, const std::string &val) const;
	virtual void SetPublicKey(Barry::Contact &con, const std::string &val) const;
	virtual void SetNotes(Barry::Contact &con, const std::string &val) const;
	virtual void SetImage(Barry::Contact &con, const std::string &val) const;
	virtual void SetWorkPostalAddress(Barry::Contact &con, const std::string &val) const;
	virtual void SetHomePostalAddress(Barry::Contact &con, const std::string &val) const;
	virtual void SetFullName(Barry::Contact &con, const std::string &val) const;
	virtual void SetFQDN(Barry::Contact &con, const std::string &val) const;


	//
	// Name heuristics
	//

	virtual void ClearHeuristics();
	virtual bool RunHeuristics(Barry::Contact &con);

	//
	// Operations
	//

	void DumpLdif(std::ostream &os, const Barry::Contact &contact) const;
	bool ReadLdif(std::istream &is, Barry::Contact &contact);
							// returns true on success
	void DumpMap(std::ostream &os) const;


	static std::string MakeLdifData(const std::string &str);
	static bool NeedsEncoding(const std::string &str);
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const ContactLdif &ldif) {
	ldif.DumpMap(os);
	return os;
}

} // namespace Barry

#endif

