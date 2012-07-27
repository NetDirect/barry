//
// \file	vtodo.cc
//		Conversion routines for vtodos (VCALENDAR, etc)
//

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2006-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <opensync/opensync.h>
#include <opensync/opensync-time.h>

#include "vtodo.h"
#include "environment.h"
#include "trace.h"
#include "tosserror.h"
#include <stdint.h>
#include <glib.h>
#include <string.h>
#include <sstream>
#include "i18n.h"

using namespace Barry::Sync;

//////////////////////////////////////////////////////////////////////////////
//

VTodoConverter::VTodoConverter()
	: m_Data(0)
{
}

VTodoConverter::VTodoConverter(uint32_t newRecordId)
	: m_Data(0),
	m_RecordId(newRecordId)
{
}

VTodoConverter::~VTodoConverter()
{
	if( m_Data )
		g_free(m_Data);
}

// Transfers ownership of m_Data to the caller
char* VTodoConverter::ExtractData()
{
	Trace trace("VTodoConverter::ExtractData");
	char *ret = m_Data;
	m_Data = 0;
	return ret;
}

bool VTodoConverter::ParseData(const char *data)
{
	Trace trace("VTodoConverter::ParseData");

	try {

		vTimeConverter vtc;
		vTodo vtodo(vtc);
		m_Task = vtodo.ToBarry(data, m_RecordId);

	}
	catch( Barry::ConvertError &ce ) {
		trace.logf(_("ERROR: vtodo:Barry::ConvertError exception: %s"), ce.what());
		m_last_errmsg = ce.what();
		return false;
	}

	return true;
}

// Barry storage operator
void VTodoConverter::operator()(const Barry::Task &rec)
{
	Trace trace("VTodoConverter::operator()");

	// Delete data if some already exists
	if( m_Data ) {
		g_free(m_Data);
		m_Data = 0;
	}

	try {

		vTimeConverter vtc;
		vTodo vtodo(vtc);
		vtodo.ToTask(rec);
		m_Data = vtodo.ExtractVTodo();

	}
	catch( Barry::ConvertError &ce ) {
		trace.logf(_("ERROR: vtodo:Barry::ConvertError exception: %s"), ce.what());
		m_last_errmsg = ce.what();
	}
}

// Barry builder operator
bool VTodoConverter::operator()(Barry::Task &rec, Barry::Builder &)
{
	Trace trace("VTodoConverter::builder operator()");

	rec = m_Task;
	return true;
}

// Handles calling of the Barry::Controller to fetch a specific
// record, indicated by index (into the RecordStateTable).
// Returns a g_malloc'd string of data containing the vevent20
// data.  It is the responsibility of the caller to free it.
// This is intended to be passed into the GetChanges() function.
char* VTodoConverter::GetRecordData(BarryEnvironment *env, unsigned int dbId,
				Barry::RecordStateTable::IndexType index)
{
	Trace trace("VTodoConverter::GetRecordData()");

	using namespace Barry;

	VTodoConverter task2todo;
	RecordParser<Task, VTodoConverter> parser(task2todo);
	env->GetDesktop()->GetRecord(dbId, index, parser);
	return task2todo.ExtractData();
}

bool VTodoConverter::CommitRecordData(BarryEnvironment *env, unsigned int dbId,
	Barry::RecordStateTable::IndexType StateIndex, uint32_t recordId,
	const char *data, bool add, std::string &errmsg)
{
	Trace trace("VTodoConverter::CommitRecordData()");

	uint32_t newRecordId;
	if( add ) {
		// use given id if possible
		if( recordId && !env->m_TodoSync.m_Table.GetIndex(recordId) ) {
			// recordId is unique and non-zero
			newRecordId = recordId;
		}
		else {
			trace.log(_("Can't use recommended recordId, generating new one."));
			newRecordId = env->m_TodoSync.m_Table.MakeNewRecordId();
		}
	}
	else {
		newRecordId = env->m_TodoSync.m_Table.StateMap[StateIndex].RecordId;
	}
	trace.logf("newRecordId: %u", newRecordId);

	VTodoConverter convert(newRecordId);
	if( !convert.ParseData(data) ) {
		std::ostringstream oss;
		oss << _("unable to parse change data for new RecordId: ")
		    << newRecordId
		    << " (" << convert.GetLastError() << ") "
		    << _("data: ") << data;
		errmsg = oss.str();
		trace.log(errmsg.c_str());
		return false;
	}

	Barry::RecordBuilder<Barry::Task, VTodoConverter> builder(convert);

	if( add ) {
		trace.log(_("adding record"));
		env->GetDesktop()->AddRecord(dbId, builder);
	}
	else {
		// we need to use a workaround for the Tasks database,
		// since there is a bug in many device firmwares which
		// causes corruption when using SetRecord().
		//
		// so instead of the nice simple:
		//
		/*
		trace.log("setting record");
		env->GetDesktop()->SetRecord(dbId, StateIndex, builder);
		trace.log("clearing dirty flag");
		env->GetDesktop()->ClearDirty(dbId, StateIndex);
		*/
		//
		// we have to delete, add, refresh the state index table,
		// and then clear the dirty flag on the new record
		//
		// but since the upper level code will clear all the
		// dirty flags for us, we can skip the state index and
		// dirty flag step, and leave it for FinishSync() in
		// barry_sync... :-)
		//
		trace.log(_("deleting task record"));
		env->GetDesktop()->DeleteRecord(dbId, StateIndex);
		trace.log(_("re-adding task record"));
		env->GetDesktop()->AddRecord(dbId, builder);
	}

	return true;
}

