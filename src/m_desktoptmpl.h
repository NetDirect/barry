///
/// \file	m_desktoptmpl.h
///		Ease of use templates for the Desktop mode class
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

#ifndef __BARRY_M_DESKTOPTMPL_H__
#define __BARRY_M_DESKTOPTMPL_H__

#include "dll.h"
#include "m_desktop.h"
#include "parser.h"
#include "builder.h"

namespace Barry { namespace Mode {

BXEXPORT void LoadDatabase(unsigned int dbId, Parser &parser);
BXEXPORT void SaveDatabase(unsigned int dbId, Builder &builder);

template <class RecordT, class StorageT>
void Desktop::LoadDatabaseByType(StorageT &store)
{
	unsigned int dbId = this->GetDBID( RecordT::GetDBName() );
	Barry::RecordParser<RecordT, StorageT> parser(store);
	this->LoadDatabase(dbId, parser);
}

template <class RecordT, class StorageT>
void Desktop::SaveDatabaseByType(StorageT &store)
{
	unsigned int dbId = this->GetDBID( RecordT::GetDBName() );
	Barry::RecordBuilder<RecordT, StorageT> build(store);
	SaveDatabase(dbId, build);
}

template <class StorageT>
void Desktop::LoadDatabaseByName(const std::string &name, StorageT &store)
{
	if( name == Contact::GetDBName() )
		LoadDatabaseByType<Contact>(store);
	else if( name == Message::GetDBName() )
		LoadDatabaseByType<Message>(store);
	else if( name == Calendar::GetDBName() )
		LoadDatabaseByType<Calendar>(store);
	else
		throw Error("Unknown database name in LoadDatabaseByName: " + name);
}

template <class StorageT>
void Desktop::SaveDatabaseByName(const std::string &name, StorageT &store)
{
	if( name == Contact::GetDBName() )
		SaveDatabaseByType<Contact>(store);
	else if( name == Message::GetDBName() )
		SaveDatabaseByType<Message>(store);
	else if( name == Calendar::GetDBName() )
		SaveDatabaseByType<Calendar>(store);
	else
		throw Error("Unknown database name in SaveDatabaseByName: " + name);
}

template <class RecordT>
void Desktop::AddRecordByType(uint32_t recordId, const RecordT &rec)
{
	unsigned int dbId = this->GetDBID( RecordT::GetDBName() );
	// FIXME - I know this is a convenience template, but it still
	// hurts making a temporary copy just to set the record ID...
	// create a better API for this.
	RecordT r = rec;
	r.SetIds(RecordT::GetDefaultRecType(), recordId);
	Barry::RecordFetch<RecordT> fetch(r);
	Barry::RecordBuilder<RecordT, Barry::RecordFetch<RecordT> > build(fetch);
	this->AddRecord(dbId, build);
}


}} // namespace Barry::Mode

#endif

