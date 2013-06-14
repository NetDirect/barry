///
/// \file	record.h
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///		This header provides the common types and classes
///		used by the general record parser classes in the
///		r_*.h files.  Only application-safe API stuff goes in
///		here.  Internal library types go in record-internal.h
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_H__
#define __BARRY_RECORD_H__

#include "dll.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <stdexcept>
	
#ifdef WINCE
/* The WinCE compiler v14.00.60131 doesn't like using operator= from base classes, so don't
 * confuse it with a using directive. */
#define USE_BASE_ASSIGNMENT_OPERATOR
#else
#define USE_BASE_ASSIGNMENT_OPERATOR using base_type::operator=;
#endif

// forward declarations
namespace Barry { class Data; }

namespace Barry {

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//


// stream-based wrapper to avoid printing strings that contain
// the \r carriage return characters
class BXEXPORT Cr2LfWrapper
{
	friend BXEXPORT std::ostream& operator<< (std::ostream &os, const Cr2LfWrapper &str);
	const std::string &m_str;
public:
	explicit Cr2LfWrapper(const std::string &str)
		: m_str(str)
	{
	}
};
BXEXPORT std::ostream& operator<< (std::ostream &os, const Cr2LfWrapper &str);

/// Struct wrapper for time_t, to make sure that it has its own type,
/// for overload purposes.  Some systems, like QNX, use a uint32_t typedef.
///
/// If Time contains 0, it is considered invalid/uninitialized when using
/// IsValid().  Validity has no affect on comparison operators.
struct BXEXPORT TimeT
{
	time_t Time;

	TimeT()
		: Time(0)
	{
	}

	explicit TimeT(time_t t)
		: Time(t)
	{
	}

	void clear()
	{
		Time = 0;
	}

	bool IsValid() const { return Time > 0; }

	bool operator< (const Barry::TimeT &other) const
	{
		return Time < other.Time;
	}

	bool operator== (const Barry::TimeT &other) const
	{
		return Time == other.Time;
	}

	bool operator!= (const Barry::TimeT &other) const
	{
		return !operator==(other);
	}
};
BXEXPORT std::ostream& operator<< (std::ostream &os, const TimeT &t);

struct BXEXPORT CommandTableCommand
{
	unsigned int Code;
	std::string Name;
};

class BXEXPORT CommandTable
{
public:
	typedef CommandTableCommand Command;
	typedef std::vector<Command> CommandArrayType;

	CommandArrayType Commands;

private:
	BXLOCAL const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);
public:
	CommandTable();
	~CommandTable();

	void Parse(const Data &data, size_t offset);
	void Clear();

	// returns 0 if unable to find command name, which is safe, since
	// 0 is a special command that shouldn't be in the table anyway
	unsigned int GetCommand(const std::string &name) const;

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const CommandTable &command) {
	command.Dump(os);
	return os;
}



struct BXEXPORT RecordStateTableState
{
	unsigned int Index;
	uint32_t RecordId;
	bool Dirty;
	unsigned int RecType;
	std::string Unknown2;
};

class BXEXPORT RecordStateTable
{
public:
	typedef RecordStateTableState State;
	typedef unsigned int IndexType;
	typedef std::map<IndexType, State> StateMapType;

	StateMapType StateMap;

private:
	mutable IndexType m_LastNewRecordId;

private:
	BXLOCAL const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	RecordStateTable();
	~RecordStateTable();

	void Parse(const Data &data);
	void Clear();

	bool GetIndex(uint32_t RecordId, IndexType *pFoundIndex = 0) const;
	uint32_t MakeNewRecordId() const;

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const RecordStateTable &rst) {
	rst.Dump(os);
	return os;
}



struct BXEXPORT DatabaseItem
{
	unsigned int Number;
	unsigned int RecordCount;
	std::string Name;
};

class BXEXPORT DatabaseDatabase
{
public:
	typedef DatabaseItem Database;
	typedef std::vector<Database> DatabaseArrayType;

	DatabaseArrayType Databases;

private:
	template <class RecordType, class FieldType>
	void ParseRec(const RecordType &rec, const unsigned char *end);

	template <class FieldType>
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end);

public:
	DatabaseDatabase();
	~DatabaseDatabase();

	void Parse(const Data &data);
	void Clear();

	void SortByName();
	void SortByRecordCount();
	unsigned int GetTotalRecordCount() const;

	// returns true on success, and fills target
	bool GetDBNumber(const std::string &name, unsigned int &number) const;
	bool GetDBName(unsigned int number, std::string &name) const;

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const DatabaseDatabase &dbdb) {
	dbdb.Dump(os);
	return os;
}

struct UnknownData
{
	std::string raw_data;

	const std::string::value_type* data() const { return raw_data.data(); }
	std::string::size_type size() const { return raw_data.size(); }
	void assign(const std::string::value_type *s, std::string::size_type n)
		{ raw_data.assign(s, n); }

	bool operator==(const UnknownData &other) const
	{
		return raw_data == other.raw_data;
	}

	bool operator!=(const UnknownData &other) const
	{
		return !operator==(other);
	}

	bool operator< (const UnknownData &other) const
	{
		return raw_data < other.raw_data;
	}
};

struct BXEXPORT UnknownField
{
	uint8_t type;
	UnknownData data;

	bool operator==(const UnknownField &other) const
	{
		return	type == other.type &&
			data == other.data;
	}

	bool operator!=(const UnknownField &other) const
	{
		return !operator==(other);
	}

	bool operator< (const UnknownField &other) const
	{
		return type < other.type && data < other.data;
	}
};
typedef std::vector<UnknownField> UnknownsType;
BXEXPORT std::ostream& operator<< (std::ostream &os, const UnknownsType &unknowns);

// simple string email type and list... keep this a simple string list,
// so it can be reused for other address-like data, like phone numbers.
// If you need something more complex, use EmailAddress below or
// create a new type.
typedef std::string				EmailType;
class BXEXPORT EmailList : public std::vector<EmailType>
{
public:
	typedef std::vector<EmailType>		base_type;

public:
	using base_type::size;
	using base_type::begin;
	using base_type::end;
	using base_type::at;
	using base_type::rbegin;
	using base_type::rend;
	using base_type::empty;
	using base_type::resize;
	using base_type::reserve;
	using base_type::front;
	using base_type::back;
	using base_type::push_back;
	using base_type::pop_back;
	using base_type::insert;
	using base_type::erase;
	using base_type::swap;
	using base_type::clear;
	using base_type::operator[];
	USE_BASE_ASSIGNMENT_OPERATOR
};
BXEXPORT std::ostream& operator<< (std::ostream &os, const EmailList &list);

// struct, attempting to combine name + email address, for mail
struct BXEXPORT EmailAddress
{
	std::string Name;
	std::string Email;

	EmailAddress()
	{
	}

	/// Converts "Name <address@host.com>" into Name + Address
	/// Will also handle just a plain address too.
	explicit EmailAddress(const std::string &complex_address);

	void clear()
	{
		Name.clear();
		Email.clear();
	}

	size_t size() const
	{
		return Name.size() + Email.size();
	}

	bool operator==(const EmailAddress &other) const
	{
		return	Name == other.Name &&
			Email == other.Email;
	}

	bool operator!=(const EmailAddress &other) const
	{
		return !operator==(other);
	}

	bool operator< (const EmailAddress &other) const
	{
		// sort by email only, since not every address has a name
		return Email < other.Email;
	}
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const EmailAddress &msga);

class BXEXPORT EmailAddressList : public std::vector<EmailAddress>
{
public:
	std::string ToCommaSeparated() const;
	void AddCommaSeparated(const std::string &list);
};

BXEXPORT std::ostream& operator<<(std::ostream &os, const EmailAddressList &elist);

struct BXEXPORT PostalAddress
{
	std::string
		Address1,
		Address2,
		Address3,
		City,
		Province,
		PostalCode,
		Country;

	std::string GetLabel() const;
	void Clear();

	bool HasData() const { return Address1.size() || Address2.size() ||
		Address3.size() || City.size() || Province.size() ||
		PostalCode.size() || Country.size(); }

	bool operator==(const PostalAddress &other) const
	{
		return	Address1 == other.Address1 &&
			Address2 == other.Address2 &&
			Address3 == other.Address3 &&
			City == other.City &&
			Province == other.Province &&
			PostalCode == other.PostalCode &&
			Country == other.Country;
	}
	bool operator!=(const PostalAddress &other) const
	{
		return !operator==(other);
	}
	bool operator< (const PostalAddress &other) const
	{
		return GetLabel() < other.GetLabel();
	}
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const PostalAddress &msga);

struct BXEXPORT Date
{
	int Month;			// 0 to 11
	int Day;			// 1 to 31
	int Year;			// exact number, eg. 2008

	Date() : Month(0), Day(0), Year(0) {}
	explicit Date(const struct tm *timep);

	bool HasData() const { return Month || Day || Year; }
	void Clear();

	void ToTm(struct tm *timep) const;
	std::string ToYYYYMMDD() const;
	std::string ToBBString() const;	// converts to Blackberry string
					// format of DD/MM/YYYY

	bool FromTm(const struct tm *timep);
	bool FromBBString(const std::string &str);
	bool FromYYYYMMDD(const std::string &str);

	bool operator==(const Date &other) const
	{
		return Month == other.Month &&
			Day == other.Day &&
			Year == other.Year;
	}
	bool operator!=(const Date &other) const
	{
		return !operator==(other);
	}
	bool operator< (const Date &other) const
	{
		// YYYYMMDD as integer
		unsigned int v1 = Year * 10000 + Month * 100 + Day;
		unsigned int v2 = other.Year * 10000 + other.Month * 100 + other.Day;
		return v1 < v2;
	}
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const Date &date);

class BXEXPORT CategoryList : public std::vector<std::string>
{
public:
	typedef std::vector<std::string>		base_type;

public:
	/// Parses the given comma delimited category string into
	/// this CategoryList object, appending each token to the vector.
	/// Will clear vector beforehand.
	void CategoryStr2List(const std::string &str);

	/// Turns the current vectory into a comma delimited category
	/// string suitable for use in Calendar, Task, and Memo
	/// protocol values.
	void CategoryList2Str(std::string &str) const;

	USE_BASE_ASSIGNMENT_OPERATOR
};
BXEXPORT std::ostream& operator<<(std::ostream &os, const CategoryList &cl);



//////////////////////////////////////////////////////////////////////////////
// Generic Field Handles

/// \addtogroup GenericFieldHandles
///		Generic field handle classes, used to reference and work
///		with record members in a flexible, indirect way.
///
///		There are two ways to access device record data.  The obvious
///		way is to instantiate a record class, such as Contact, and
///		access the public data members of that class to read and
///		write.  If you always work with the same record class, this
///		works fine.
///
///		The other way is to have a list of pointers to members.
///		For example, you may wish to compare two records, without
///		actually caring what data is in each one.  You can compare
///		at the record class level, with Contact one, two; and then
///		if( one == two ), but this will not tell you what field in
///		the record changed.
///
///		This last feature is what Generic Field Handles are meant to
///		fix.  Each record class will contain a GetFieldHandles()
///		member function, which will return a list of type
///		FieldHandle<T>::ListT (currently a std::vector<>)
///		objects, for that specific record.  For example, Contact
///		would fill the ListT with FieldHandle<Contact> objects.
///		Each FieldHandle<> object contains a C++ pointer-to-member,
///		which the FieldHandle refers to, as well as a FieldIdentity
///		object.  The FieldIdentity object contains various identitying
///		information, such as the C++ variable name, an English
///		(or localized language) display name of the field, suitable
///		for user prompts, and other data more useful to the library.
///
///		The FieldHandle<> object has two member functions: Value()
///		and Member().
///
///		Value() will call a callback function with the _value_ of
///		the variable that FieldHandle<> points to.  For example,
///		if the FieldHandle<> points to a std::string record member
///		variable, then Value() will pass that string value in as
///		an argument, along with a reference to the FieldIdentity
///		object.  Value() requires a callback object and a record
///		object to perform this callback.
///
///		Member() will call a callback function/functor with the
///		pointer-to-member pointer and the FieldIdentity object.
///		This allows the programmer to create a functor with multiple
///		record objects, perhaps two objects to compare individual
///		fields, and use the pointer-to-member to access the field
///		data as needed.
///
///		For now, all data and callbacks are const, meaning that it
///		is not possible (without const_casting) to write to the
///		record via the pointers-to-members.  This design decision
///		may need to be revisited someday, depending on its usefulness.
///
/// @{

//
// FieldIdentity
//
/// This class holds data that identifies a given field in a record.
/// This is fairly constant data, referenced by the FieldHandle class.
/// The information in here should be enough to show the user what kind
/// of field this is.
///
struct BXEXPORT FieldIdentity
{
	// useful public data
	const char *Name;		// C++ name of field member variable in
					// record class
	std::string DisplayName;	// localized display name of field
					// in UTF8
					// FIXME - should we leave localization
					// to the application?

	// subfield detection
	bool HasSubfields;		// true if this field has subfields
	const char *ParentName;		// name of field member variable that
					// this field is a member of, or NULL
					// if this field is a member of the
					// record class.
					// For example, Contact::PostalAddress
					// would have HasSubfields == true,
					// and ParentName == NULL, while all
					// its subfield strings would have
					// HasSubfields == false and
					// ParentName == "WorkAddress" or
					// "HomeAddress".
					// The application could then decide
					// whether to process only main fields,
					// some of which have subfields,
					// or only individual subfields.

	// internal field data
	int FieldTypeCode;		// device type code for this field.
					// if -1, then this is a conglomerate
					// C++ field, such as
					// Contact::PostalAddress, not a device
					// field.
					// If -1, then none of the following
					// fields are valid.
	const char *Ldif;		// matching LDIF field name, or NULL
	const char *ObjectClass;	// matching LDIF object class, or NULL
	bool IconvNeeded;		// if true, the device's data needs to
					// be passed through an IConverter

	FieldIdentity(const char *name, const std::string &display_name,
		int type_code = -1,
		bool iconvneeded = false,
		const char *ldif = 0, const char *oclass = 0,
		bool has_sub = false, const char *parent = 0
		)
		: Name(name)
		, DisplayName(display_name)
		, HasSubfields(has_sub)
		, ParentName(parent)
		, FieldTypeCode(type_code)
		, Ldif(ldif)
		, ObjectClass(oclass)
		, IconvNeeded(iconvneeded)
	{
	}
};

//
// EnumConstants
//
/// This is the base class for the hierarchy of classes to define
/// enum record members.  This is the base class, which contains the
/// common code for creating and defining a list of enum constants for
/// a given enum field.  The next derived class is EnumFieldBase<RecordT>,
/// which defines the virtual API for talking to a given enum field
/// in a given record.  The next derived class is EnumField<RecordT, EnumT>,
/// which implements the pointer-to-member and virtual API for a given
/// enum type in a given record class.
///
/// For example, the Bookmark record class has the following enum field:
///
/// <pre>
///	enum BrowserIdentityType
///	{
///		IdentityAuto = 0,
///		IdentityBlackBerry,
///		IdentityFireFox,
///		IdentityInternetExplorer,
///		IdentityUnknown
///	};
///	BrowserIdentityType BrowserIdentity;
/// </pre>
///
/// The EnumConstants class will hold a vector of EnumConstant structs
/// defining each of the identity constants: Auto, BlackBerry, FireFox,
/// InternetExplorer, and Unknown.
///
/// The derived class EnumFieldBase<Bookmark> will define two additional
/// pure virtual API calls: GetValue(const Bookmark&) and
/// SetValue(Bookmark&, int).
///
/// Finally, the derived class EnumField<Bookmark,Bookmark::BrowserIdentityType>
/// will implement the virtual API, and contain a pointer-to-member to
/// the Bookmark::BrowserIdentity member field.
///
/// The FieldHandle<Bookmark> class will hold a pointer to
/// EnumFieldBase<Bookmark>, which can hold a pointer to a specific
/// EnumField<> object, one object for each of Bookmark's enum types,
/// of which there are currently 3.
///
class BXEXPORT EnumConstants
{
public:
	/// This defines one of the enum constants being defined.
	/// For example, for an enum declaration like:
	/// enum Mine { A, B, C }; then this struct could contain
	/// a definition for A, B, or C, but only one at at time.
	/// All three would be defined by the EnumConstantList.
	struct EnumConstant
	{
		const char *Name;		//< C++ name of enum constant
		std::string DisplayName;	//< user-friendly name / meaning
		int Value;			//< constant enum value

		EnumConstant(const char *name, const std::string &display,
			int value)
			: Name(name)
			, DisplayName(display)
			, Value(value)
		{
		}
	};

	typedef std::vector<EnumConstant>	EnumConstantList;

private:
	EnumConstantList m_constants;

public:
	virtual ~EnumConstants() {}

	/// Adds a constant definition to the list
	void AddConstant(const char *name, const std::string &display, int val);

	/// Returns a vector of EnumConstant objects, describing all enum
	/// constants valid for this enum field.
	const EnumConstantList& GetConstantList() const { return m_constants; }

	/// Returns the EnumConstant for the given value.
	/// Throws std::logic_error if not found.
	const EnumConstant& GetConstant(int value) const;

	/// Returns the constant name (C++ name) based on the given value.
	/// Throws std::logic_error if not found.
	const char* GetName(int value) const;

	/// Returns the display name based on the given value.
	/// Throws std::logic_error if not found.
	const std::string& GetDisplayName(int value) const;

	/// Returns true if the value matches one of the constants in the list.
	bool IsConstantValid(int value) const;
};

//
// FieldValueHandlerBase
//
/// This is a pure virtual base class, defining the various types that
/// record fields can be.  To be able to handle all the types of data
/// in all records, override these virtual functions to do with the
/// data as you wish.
///
/// All data from the records and fields will be passed in by value.
/// i.e. if field is string data, the overloaded std::string handler
/// will be called, and a refernce to the string will be passed in.
///
/// The advantage of using this virtual class is that less code will be
/// generated by templates.  The disadvantage is that this is less flexible.
/// You will only get called for one field and record at a time.
/// So you can't do comparisons this way.
///
class BXEXPORT FieldValueHandlerBase
{
public:
	virtual ~FieldValueHandlerBase() {}

	/// For type std::string
	virtual void operator()(const std::string &v,
				const FieldIdentity &id) const = 0;
	/// For type EmailAddressList
	virtual void operator()(const EmailAddressList &v,
				const FieldIdentity &id) const = 0;
	/// For type Barry::TimeT
	virtual void operator()(const Barry::TimeT &v,
				const FieldIdentity &id) const = 0;
	/// For type uint8_t
	virtual void operator()(const uint8_t &v,
				const FieldIdentity &id) const = 0;
	/// For type uint16_t
	virtual void operator()(const uint16_t &v,
				const FieldIdentity &id) const = 0;
	/// For type uint32_t
	virtual void operator()(const uint32_t &v,
				const FieldIdentity &id) const = 0;
	/// For type uint64_t
	virtual void operator()(const uint64_t &v,
				const FieldIdentity &id) const = 0;
	/// For type bool
	virtual void operator()(const bool &v,
				const FieldIdentity &id) const = 0;
	/// For type int32_t
	virtual void operator()(const int32_t &v,
				const FieldIdentity &id) const = 0;
	/// For type EmailList
	virtual void operator()(const EmailList &v,
				const FieldIdentity &id) const = 0;
	/// For type Date
	virtual void operator()(const Date &v,
				const FieldIdentity &id) const = 0;
	/// For type CategoryList
	virtual void operator()(const CategoryList &v,
				const FieldIdentity &id) const = 0;
	/// For type PostalAddress
	virtual void operator()(const PostalAddress &v,
				const FieldIdentity &id) const = 0;
	/// For type UnknownsType
	virtual void operator()(const UnknownsType &v,
				const FieldIdentity &id) const = 0;
};

///
/// EnumFieldBase<RecordT>
///
template <class RecordT>
class EnumFieldBase : public EnumConstants
{
public:
	/// Return value of enum in rec
	virtual int GetValue(const RecordT &rec) const = 0;
	/// Set value of enum in rec
	/// Throws std::logic_error if value is out of range
	virtual void SetValue(RecordT &rec, int value) = 0;
};

///
/// EnumField<RecordT, EnumT>
///
template <class RecordT, class EnumT>
class EnumField : public EnumFieldBase<RecordT>
{
	EnumT RecordT::* m_mp;

public:
	explicit EnumField(EnumT RecordT::* mp)
		: m_mp(mp)
	{
	}

	virtual int GetValue(const RecordT &rec) const
	{
		return rec.*m_mp;
	}

	virtual void SetValue(RecordT &rec, int value)
	{
		if( !this->IsConstantValid(value) )
			throw std::logic_error("Bad enum value in EnumField");
		rec.*m_mp = (EnumT) value;
	}
};

//
// FieldHandle<RecordT>
//
/// This is a template class that handles pointers to members of multiple
/// types of data and multiple types of records.
///
/// This class contains a union of all known data pointers in all records.
/// Therefore this class can hold a pointer to member of any record class.
///
/// To do something with the field that this FieldHandle<> class refers to,
/// call either Value() or Member() with appropriate callback functors.
/// Value will pass a reference to the field.  You can use an object
/// derived from FieldValueHandlerBase here.  Member() will pass a pointer
/// to member.  Your functor will need to contain the record data in order
/// to access its data via the pointer to member.
///
/// The template functor callback that you pass into member must be
/// capable of this:
///
/// <pre>
///	template &lt;class RecordT&gt;
///	struct Callback
///	{
///		RecordT m_rec;
///
///		void operator()(typename FieldHandle<RecordT>::PostalPointer pp,
///			const FieldIdentity &id) const
///		{
///			PostalAddress pa = m_rec.*(pp.m_PostalAddress);
///			std::string val = pa.*(pp.m_PostalField);
///			...
///		}
///
///		template &lt;class TypeT&gt;
///		void operator()(TypeT RecordT::* mp,
///				const FieldIdentity &id) const
///		{
///			TypeT val = m_rec.*mp;
///			...
///		}
///	};
/// </pre>
///
/// You don't have to use a TypeT template, but if you don't, then you must
/// support all field types that the record class you're processing uses.
///
///
template <class RecordT>
class FieldHandle
{
public:
	typedef FieldHandle<RecordT>			Self;
	typedef std::vector<Self>			ListT;

	// Need to use this in the union, so no constructor allowed
	struct PostalPointer
	{
		PostalAddress RecordT::* m_PostalAddress;
		std::string PostalAddress::* m_PostalField;
	};

	// So use a factory function
	static PostalPointer MakePostalPointer(PostalAddress RecordT::* p1,
			std::string PostalAddress::* p2)
	{
		PostalPointer pp;
		pp.m_PostalAddress = p1;
		pp.m_PostalField = p2;
		return pp;
	}

private:
	union PointerUnion
	{
		std::string RecordT::* m_string;		// index 0
		EmailAddressList RecordT::* m_EmailAddressList; // 1
		Barry::TimeT RecordT::* m_time;			// 2
		PostalPointer m_postal;				// 3
		uint8_t RecordT::* m_uint8;			// 4
		uint32_t RecordT::* m_uint32;			// 5
		EmailList RecordT::* m_EmailList;		// 6
		Date RecordT::* m_Date;				// 7
		CategoryList RecordT::* m_CategoryList;		// 8
//		GroupLinksType RecordT::* m_GroupLinksType;	// 9
		UnknownsType RecordT::* m_UnknownsType;		// 10
		bool RecordT::* m_bool;				// 11
		uint64_t RecordT::* m_uint64;			// 12
		uint16_t RecordT::* m_uint16;			// 13
		PostalAddress RecordT::* m_PostalAddress;	// 14
		// used by non-union m_enum below:		// 15
		int32_t RecordT::* m_int32;			// 16
	};

	int m_type_index;
	PointerUnion m_union;
	EnumFieldBase<RecordT> *m_enum;	// never freed, since this is a
					// static list, existing to end of
					// program lifetime

	FieldIdentity m_id;

public:
	// 0
	FieldHandle(std::string RecordT::* mp, const FieldIdentity &id)
		: m_type_index(0)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_string = mp;
	}

	// 1
	FieldHandle(EmailAddressList RecordT::* mp, const FieldIdentity &id)
		: m_type_index(1)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_EmailAddressList = mp;
	}

	// 2
	FieldHandle(Barry::TimeT RecordT::* mp, const FieldIdentity &id)
		: m_type_index(2)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_time = mp;
	}

	// 3
	FieldHandle(const PostalPointer &pp, const FieldIdentity &id)
		: m_type_index(3)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_postal = pp;
	}

	// 4
	FieldHandle(uint8_t RecordT::* mp, const FieldIdentity &id)
		: m_type_index(4)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_uint8 = mp;
	}

	// 5
	FieldHandle(uint32_t RecordT::* mp, const FieldIdentity &id)
		: m_type_index(5)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_uint32 = mp;
	}

	// 6
	FieldHandle(EmailList RecordT::* mp, const FieldIdentity &id)
		: m_type_index(6)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_EmailList = mp;
	}

	// 7
	FieldHandle(Date RecordT::* mp, const FieldIdentity &id)
		: m_type_index(7)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_Date = mp;
	}

	// 8
	FieldHandle(CategoryList RecordT::* mp, const FieldIdentity &id)
		: m_type_index(8)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_CategoryList = mp;
	}

	// 9
//	FieldHandle(GroupLinksType RecordT::* mp, const FieldIdentity &id)
//		: m_type_index(9)
//		, m_enum(0)
//		, m_id(id)
//	{
//		m_union.m_GroupLinksType = mp;
//	}

	// 10
	FieldHandle(UnknownsType RecordT::* mp, const FieldIdentity &id)
		: m_type_index(10)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_UnknownsType = mp;
	}

	// 11
	FieldHandle(bool RecordT::* mp, const FieldIdentity &id)
		: m_type_index(11)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_bool = mp;
	}

	// 12
	FieldHandle(uint64_t RecordT::* mp, const FieldIdentity &id)
		: m_type_index(12)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_uint64 = mp;
	}

	// 13
	FieldHandle(uint16_t RecordT::* mp, const FieldIdentity &id)
		: m_type_index(13)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_uint16 = mp;
	}

	// 14
	FieldHandle(PostalAddress RecordT::* mp, const FieldIdentity &id)
		: m_type_index(14)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_PostalAddress = mp;
	}

	// 15
	FieldHandle(EnumFieldBase<RecordT> *enum_, const FieldIdentity &id)
		: m_type_index(15)
		, m_enum(enum_)
		, m_id(id)
	{
	}

	// 16
	FieldHandle(int32_t RecordT::* mp, const FieldIdentity &id)
		: m_type_index(16)
		, m_enum(0)
		, m_id(id)
	{
		m_union.m_int32 = mp;
	}

	/// Extracts FieldIdentity object from FieldHandle<>
	const FieldIdentity& GetIdentity() const { return m_id; }

	/// Calls the matching virtual function in FieldValueHandlerBase,
	/// passing in the value of the field that this FieldHandle<>
	/// refers to, and a referenct to the FieldIdentity object.
	/// Caller must pass in a RecordT object as well.
	void Value(const FieldValueHandlerBase &vh, const RecordT &rec) const
	{
		switch( m_type_index )
		{
		case 0:
			vh(rec.*(m_union.m_string), m_id);
			break;
		case 1:
			vh(rec.*(m_union.m_EmailAddressList), m_id);
			break;
		case 2:
			vh(rec.*(m_union.m_time), m_id);
			break;
		case 3:
			vh(rec.*(m_union.m_postal.m_PostalAddress).*(m_union.m_postal.m_PostalField), m_id);
			break;
		case 4:
			vh(rec.*(m_union.m_uint8), m_id);
			break;
		case 5:
			vh(rec.*(m_union.m_uint32), m_id);
			break;
		case 6:
			vh(rec.*(m_union.m_EmailList), m_id);
			break;
		case 7:
			vh(rec.*(m_union.m_Date), m_id);
			break;
		case 8:
			vh(rec.*(m_union.m_CategoryList), m_id);
			break;
//		case 9:
//			vh(rec.*(m_union.m_GroupLinksType), m_id);
//			break;
		case 10:
			vh(rec.*(m_union.m_UnknownsType), m_id);
			break;
		case 11:
			vh(rec.*(m_union.m_bool), m_id);
			break;
		case 12:
			vh(rec.*(m_union.m_uint64), m_id);
			break;
		case 13:
			vh(rec.*(m_union.m_uint16), m_id);
			break;
		case 14:
			vh(rec.*(m_union.m_PostalAddress), m_id);
			break;
		case 15:
			vh(m_enum->GetValue(rec), m_id);
			break;
		case 16:
			vh(rec.*(m_union.m_int32), m_id);
			break;
		default:
			throw std::logic_error("Unknown field handle type index");
		}
	}

	/// Calls the callback functor with two arguments: the pointer to
	/// member that this FieldHandle<> contains, and the FieldIdentity
	/// object.  It is assumed that the functor will either contain
	/// or know where to find one or more records of type RecordT.
	template <class CallbackT>
	void Member(const CallbackT &func) const
	{
		switch( m_type_index )
		{
		case 0:
			func(m_union.m_string, m_id);
			break;
		case 1:
			func(m_union.m_EmailAddressList, m_id);
			break;
		case 2:
			func(m_union.m_time, m_id);
			break;
		case 3:
			func(m_union.m_postal, m_id);
			break;
		case 4:
			func(m_union.m_uint8, m_id);
			break;
		case 5:
			func(m_union.m_uint32, m_id);
			break;
		case 6:
			func(m_union.m_EmailList, m_id);
			break;
		case 7:
			func(m_union.m_Date, m_id);
			break;
		case 8:
			func(m_union.m_CategoryList, m_id);
			break;
//		case 9:
//			func(m_union.m_GroupLinksType, m_id);
//			break;
		case 10:
			func(m_union.m_UnknownsType, m_id);
			break;
		case 11:
			func(m_union.m_bool, m_id);
			break;
		case 12:
			func(m_union.m_uint64, m_id);
			break;
		case 13:
			func(m_union.m_uint16, m_id);
			break;
		case 14:
			func(m_union.m_PostalAddress, m_id);
			break;
		case 15:
			func(m_enum, m_id);
			break;
		case 16:
			func(m_union.m_int32, m_id);
			break;
		default:
			throw std::logic_error("Unknown field handle type index");
		}
	}
};

/// Factory function to create a FieldHandle<> object.
template <class RecordT, class TypeT>
FieldHandle<RecordT> MakeFieldHandle(TypeT RecordT::* tp,
					const FieldIdentity &id)
{
	return FieldHandle<RecordT>(tp, id);
}

/// Calls FieldHandle<>::Member() for each defined field for a given record
/// type.  Takes a FieldHandle<>::ListT containing FieldHandle<> objects,
/// and calls Member(func) for each one.
template <class HandlesT, class CallbackT>
void ForEachField(const HandlesT &handles, const CallbackT &func)
{
	typename HandlesT::const_iterator
		b = handles.begin(),
		e = handles.end();
	for( ; b != e; ++b ) {
		b->Member(func);
	}
}

/// Calls FieldHandle<>::Value() for each defined field for a given record.
/// Takes a RecordT object and calls Value(vh, rec) for each FieldHandle<>
/// object in the record's FieldHandles set.
template <class RecordT>
void ForEachFieldValue(const RecordT &rec, const FieldValueHandlerBase &vh)
{
	typename FieldHandle<RecordT>::ListT::const_iterator
		b = RecordT::GetFieldHandles().begin(),
		e = RecordT::GetFieldHandles().end();
	for( ; b != e; ++b ) {
		b->Value(vh, rec);
	}
}

//
// FieldHandle setup macros
//
// #undef and #define the following macros to override these macros for you:
//
//	CONTAINER_OBJECT_NAME - the new FieldHandles will be .push_back()'d into
//				this container
//	RECORD_CLASS_NAME     - the name of the record class you are defining,
//				i.e. Barry::Contact, or Barry::Calendar
//

// plain field, no connection to device field
#define FHP(name, display) \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>(&RECORD_CLASS_NAME::name, \
			FieldIdentity(#name, display)))
// record field with direct connection to device field, no LDIF data
#define FHD(name, display, type_code, iconv) \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>(&RECORD_CLASS_NAME::name, \
			FieldIdentity(#name, display, type_code, iconv, \
				0, 0)))
// record field with direct connection to device field, with LDIF data
#define FHL(name, display, type_code, iconv, ldif, oclass) \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>(&RECORD_CLASS_NAME::name, \
			FieldIdentity(#name, display, type_code, iconv, \
				ldif, oclass)))
// a subfield of a conglomerate field, with direct connection to device field,
// with LDIF data
#define FHS(name, subname, display, type, iconv, ldif, oclass) \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>( \
			FieldHandle<RECORD_CLASS_NAME>::MakePostalPointer( \
				&RECORD_CLASS_NAME::name, \
				&PostalAddress::subname), \
			FieldIdentity(#name "::" #subname, display, \
				type, iconv, ldif, oclass, \
				false, #name)))
// record conglomerate field, which has subfields
#define FHC(name, display) \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>(&RECORD_CLASS_NAME::name, \
			FieldIdentity(#name, display, \
				-1, false, 0, 0, true, 0)))
// create a new EnumField<> and add it to the list... use the new_var_name
// to add constants with FHE_CONST below
#define FHE(new_var_name, record_field_type, record_field_name, display) \
	EnumField<RECORD_CLASS_NAME, RECORD_CLASS_NAME::record_field_type> \
		*new_var_name = new \
		EnumField<RECORD_CLASS_NAME, RECORD_CLASS_NAME::record_field_type> \
			(&RECORD_CLASS_NAME::record_field_name); \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>(new_var_name, \
			FieldIdentity(#record_field_name, display)))
// same as FHE, but for when RECORD_CLASS_NAME is a template argument
#define FHET(new_var_name, record_field_type, record_field_name, display) \
	EnumField<RECORD_CLASS_NAME, typename RECORD_CLASS_NAME::record_field_type> \
		*new_var_name = new \
		EnumField<RECORD_CLASS_NAME, typename RECORD_CLASS_NAME::record_field_type> \
			(&RECORD_CLASS_NAME::record_field_name); \
	CONTAINER_OBJECT_NAME.push_back( \
		FieldHandle<RECORD_CLASS_NAME>(new_var_name, \
			FieldIdentity(#record_field_name, display)))
// add constant to enum created above
#define FHE_CONST(var, name, display) \
	var->AddConstant(#name, display, RECORD_CLASS_NAME::name)

/// @}

} // namespace Barry


/// \addtogroup RecordParserClasses
///		Parser and data storage classes.  These classes take a
///		Database Database record and convert them into C++ objects.
///		Each of these classes are safe to be used in standard
///		containers, and are meant to be used in conjunction with the
///		RecordParser<> template when calling Controller::LoadDatabase().
/// @{
/// @}

#ifndef __BARRY_LIBRARY_BUILD__
// Include all parser classes, to make it easy for the application to use.
#include "r_calendar.h"
#include "r_calllog.h"
#include "r_bookmark.h"
#include "r_contact.h"
#include "r_cstore.h"
#include "r_memo.h"
#include "r_message.h"
#include "r_servicebook.h"
#include "r_task.h"
#include "r_pin_message.h"
#include "r_saved_message.h"
#include "r_sms.h"
#include "r_folder.h"
#include "r_timezone.h"
#include "r_hhagent.h"
#endif

#endif

