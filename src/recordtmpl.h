///
/// \file	recordtmpl.h
///		Standalone templates related to the record classes.
///		Split into a separate file to speed compile times.
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

#ifndef __BARRY_RECORD_TEMPLATES_H__
#define __BARRY_RECORD_TEMPLATES_H__

#include "dll.h"
#include <iosfwd>
#include <iostream>
#include <sstream>

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// Generic Field Handles

/// \addtogroup GenericFieldHandles
/// @{

//////////////////////////////////////////////////////////////////////////////
// *NamedFieldCmp classes

/// FieldSorter<> is a helper class for NamedFieldCmp<>, used as the
/// callback for FieldHandle<>::Member().  It uses operator< to store
/// a comparison result for the given record and field.
template <class RecordT>
class FieldSorter
{
	const RecordT &m_one, &m_two;
	mutable bool m_comparison, m_equal;

public:
	FieldSorter(const RecordT &a, const RecordT &b)
		: m_one(a)
		, m_two(b)
		, m_comparison(false)
		, m_equal(false)
	{
	}

	bool GetComparison() const { return m_comparison; }
	bool IsEqual() const { return m_equal; }

	void operator()(EnumFieldBase<RecordT> *ep,
		const FieldIdentity &id) const
	{
		m_comparison = ep->GetValue(m_one) < ep->GetValue(m_two);
		m_equal = !m_comparison &&
			!(ep->GetValue(m_two) < ep->GetValue(m_one));
	}

	void operator()(typename FieldHandle<RecordT>::PostalPointer pp,
		const FieldIdentity &id) const
	{
		const std::string
			&a = m_one.*(pp.m_PostalAddress).*(pp.m_PostalField),
			&b = m_two.*(pp.m_PostalAddress).*(pp.m_PostalField);

		m_comparison = a < b;
		m_equal = !m_comparison && !(b < a);
	}

	template <class TypeT>
	void operator()(TypeT RecordT::* mp, const FieldIdentity &id) const
	{
		m_comparison = m_one.*mp < m_two.*mp;
		m_equal = !m_comparison && !(m_two.*mp < m_one.*mp);
	}
};

//
// NamedFieldCmp<>
//
/// A comparison functor, intended to be used in std::sort(), which
/// allows sorting by a particular record's member variable, selected
/// by string name.  eg. It allows you to sort a vector of Contact
/// records by Name, HomeAddress, WorkPhone, or Company name, etc.
///
/// This template takes the record type as template argument, and works
/// with only that record type.
///
/// If the given name is not found the FieldHandles for RecordT, this
/// class will throw a std::logic_error exception.
///
template <class RecordT>
class NamedFieldCmp
{
	const std::string &m_name;

public:
	NamedFieldCmp(const std::string &field_name)
		: m_name(field_name)
	{
	}

	bool operator() (const RecordT &a, const RecordT &b) const
	{
		bool comparison = false;
		std::string token;
		std::istringstream iss(m_name);
		while( std::getline(iss, token, ',') ) {
			typename FieldHandle<RecordT>::ListT::const_iterator
				fhi = RecordT::GetFieldHandles().begin(),
				fhe = RecordT::GetFieldHandles().end();

			for( ; fhi != fhe; ++fhi ) {
				if( token == fhi->GetIdentity().Name ) {
					FieldSorter<RecordT> fs(a, b);
					fhi->Member(fs);
					comparison = fs.GetComparison();
					if( fs.IsEqual() ) {
						// if equal, compare next token
						break;
					}
					else {
						// done!
						return comparison;
					}
				}
			}

			if( fhi == fhe )
				throw std::logic_error("NamedFieldCmp: No field named '" + token + "' in '" + RecordT::GetDBName() + "'");
		}

		// return last seen comparison
		return comparison;
	}
};

class IConverter;
class DBData;

//
// DBNamedFieldCmp
//
/// This class is a wrapper around the NamedFieldCmp<>, allowing you to
/// sort a vector (or other container) of DBData objects.  This class will
/// parse each record before passing the result on to NamedFieldCmp<>
/// and returning the result.  All the parsing work is then thrown away,
/// so this is more of a convenience class than for performance.
///
/// This class expects that all the DBData given to it is one of the
/// known records which have a parser.  If the record unrecognized, it
/// will throw a std::logic_error exception.
///
class DBNamedFieldCmp
{
	const std::string &m_name;
	const IConverter *m_ic;

public:
	explicit DBNamedFieldCmp(const std::string &field_name,
			const Barry::IConverter *ic = 0);

	bool operator() (const Barry::DBData &a, const Barry::DBData &b) const;
};

/// @}

} // namespace Barry

#endif

