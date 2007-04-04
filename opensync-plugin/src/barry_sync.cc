//
// \file	barry_sync.cc
//		Opensync module for the USB Blackberry handheld
//

/*
    Copyright (C) 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "barry_sync.h"
#include <string>
#include <glib.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>

// All functions that are callable from outside must look like C
extern "C" {
	static void *initialize(OSyncMember *member, OSyncError **error);
	static void connect(OSyncContext *ctx);
	static void get_changeinfo(OSyncContext *ctx);
	static void batch_commit_vevent20(OSyncContext *ctx,
			OSyncContext **contexts,
			OSyncChange **changes);
	static void sync_done(OSyncContext *ctx);
	static void disconnect(OSyncContext *ctx);
	static void finalize(void *data);
	void get_info(OSyncEnv *env);
}


//////////////////////////////////////////////////////////////////////////////
//
// Support functions and classes
//

// searches for key in vdata
static std::string ExtractKeyData(const char *vdata, const char *key)
{
	int keylen = strlen(key);
	bool found = false, first = true;

	// we must do this in a case-insensitive manner, as per RFC,
	// and unfortunately there is no standard way to do it,
	// or we could be using stristr or something...
	while( *vdata ) {
		if( *vdata == '\n' || first ) {
			first = false;

			// search for end of key name
			const char *vend = ++vdata;
			while( *vend && *vend != ':' )
				vend++;

			// compare
			if( keylen == (vend - vdata) ) {
				if( strncasecmp(vdata, key, keylen) == 0 ) {
					found = true;
					vdata = vend + 1;
					break;
				}
			}
			else {
				vdata = vend;
			}
		}
		else {
			vdata++;
		}
	}

	if( found ) {
		// pack up key data for return
		const char *vend = vdata;
		while( *vend && *vend != '\r' && *vend != '\n' )
			vend++;

		return std::string(vdata, vend);
	}
	else {
		return std::string();
	}
}

class VEventConverter
{
	char *m_Data;
	std::string m_start, m_end, m_subject;
	uint32_t m_RecordId;

public:
	VEventConverter()
		: m_Data(0)
	{
	}

	explicit VEventConverter(uint32_t newRecordId)
		: m_RecordId(newRecordId)
	{
	}

	~VEventConverter()
	{
		if( m_Data )
			g_free(m_Data);
	}

	// Transfers ownership of m_Data to the caller
	char* ExtractData()
	{
		Trace trace("VEventConverter::ExtractData");
		char *ret = m_Data;
		m_Data = 0;
		return ret;
	}

	bool ParseData(const char *data)
	{
		Trace trace("VEventConverter::ParseData");

		m_start = ExtractKeyData(m_Data, "DTSTART");
		trace.log(m_start.c_str());
		m_end = ExtractKeyData(m_Data, "DTEND");
		trace.log(m_end.c_str());
		m_subject = ExtractKeyData(m_Data, "SUMMARY");
		trace.log(m_subject.c_str());

		if( !m_start.size() || !m_end.size() || !m_subject.size() )
			return false;
		return true;
	}

	// storage operator
	void operator()(const Barry::Calendar &rec)
	{
		Trace trace("VEventConverter::operator()");

		// Delete data if some already exists
		if( m_Data ) {
			g_free(m_Data);
			m_Data = 0;
		}

		// Put calendar event data into vevent20 format
		char *start = osync_time_unix2vtime(&rec.StartTime);
		char *end = osync_time_unix2vtime(&rec.EndTime);
		// FIXME - need the notification time too... where does that fit in VCALENDAR?
		m_Data = g_strdup_printf(
			"BEGIN:VCALENDAR\r\n"
			"PRODID:-//OpenSync//NONSGML Barry Calendar Record//EN\r\n"
			"VERSION:2.0\r\n"
			"BEGIN:VEVENT\r\n"
			"DTSTART:%s\r\n"
			"DTEND:%s\r\n"
			"SEQUENCE:0\r\n"
			"SUMMARY:%s\r\n"
			"END:VEVENT\r\n"
			"END:VCALENDAR",
			start, end, rec.Subject.c_str());
		g_free(start);
		g_free(end);
	}

	// builder operator
	bool operator()(Barry::Calendar &rec, unsigned int dbId)
	{
		Trace trace("VEventConverter::builder operator()");

		// FIXME - we are assuming that any non-UTC timestamps
		// in the vcalendar record will be in the current timezone...
		// Also, ParseData() currently ignores any time zone
		// parameters that might be in the vcalendar format,
		// so we can't base it on input data.
		time_t now = time(NULL);
		int zoneoffset = osync_time_timezone_diff(localtime(&now));

		rec.SetIds(Barry::Calendar::GetDefaultRecType(), m_RecordId);
		rec.StartTime = osync_time_vtime2unix(m_start.c_str(), zoneoffset);
		rec.EndTime = osync_time_vtime2unix(m_end.c_str(), zoneoffset);
		rec.Subject = m_subject;
		return true;
	}

	// Handles calling of the Barry::Controller to fetch a specific
	// record, indicated by index (into the RecordStateTable).
	// Returns a g_malloc'd string of data containing the vevent20
	// data.  It is the responsibility of the caller to free it.
	// This is intended to be passed into the GetChanges() function.
	static char* GetRecordData(BarryEnvironment *env, unsigned int index)
	{
		using namespace Barry;

		VEventConverter cal2event;
		RecordParser<Calendar, VEventConverter> parser(cal2event);
		unsigned int dbId = env->m_pCon->GetDBID("Calendar");
		env->m_pCon->GetRecord(dbId, index, parser);
		return cal2event.ExtractData();
	}

	static bool SetRecordData(BarryEnvironment *env, unsigned int index,
		const char *data);
};

void GetChanges(OSyncContext *ctx, BarryEnvironment *env,
		BarryEnvironment::cache_type &cache,
		const char *DBDBName, const char *FormatName,
		char* (*getdata)(BarryEnvironment *, unsigned int))
{
	Trace trace("GetChanges");

	// shortcut references
	using namespace Barry;
	using Barry::RecordStateTable;
	Controller &con = *env->m_pCon;

	// fetch state table
	unsigned int dbId = con.GetDBID(DBDBName);
	RecordStateTable table;
	con.GetRecordStateTable(dbId, table);

	// cycle through the state table...
	//    - if not in cache, it is added.
	//    - if in cache, check Blackberry's dirty flag
	RecordStateTable::StateMapType::const_iterator i = table.StateMap.begin();
	for( ; i != table.StateMap.end(); ++i ) {

		OSyncChange *change = 0;
		const RecordStateTable::IndexType &index = i->first;
		const RecordStateTable::State &state = i->second;

		// search the cache
		BarryEnvironment::cache_type::const_iterator c = cache.find(state.RecordId);
		if( c == cache.end() ) {
			// not in cache, this is a new item
			trace.log("found an ADDED change");
			change = osync_change_new();
			osync_change_set_changetype(change, CHANGE_ADDED);
		}
		else {
			// in the cache... dirty?
			if( state.Dirty ) {
				// modified
				trace.log("found a MODIFIED change");
				change = osync_change_new();
				osync_change_set_changetype(change, CHANGE_MODIFIED);
			}
			else {
				trace.log("no change detected");
			}
		}

		// finish filling out the change object
		if( change ) {
			osync_change_set_member(change, env->member);
			osync_change_set_objformat_string(change, FormatName);

			char *uid = g_strdup_printf("%u", state.RecordId);
			osync_change_set_uid(change, uid);
			trace.log(uid);
			g_free(uid);

			// Now you can set the data for the object
			// Set the last argument to FALSE if the real data
			// should be queried later in a "get_data" function
			char *data = (*getdata)(env, index);
			osync_change_set_data(change, data, strlen(data), TRUE);

			// just report the change via
			osync_context_report_change(ctx, change);
		}
	}

	// now cycle through the cache... any objects in the cache
	// but not found in the state table means that they have been
	// deleted in the device
	BarryEnvironment::cache_type::const_iterator c = cache.begin();
	for( ; c != cache.end(); ++c ) {
		uint32_t recordId = c->first;

		// search the state table
		i = table.StateMap.begin();
		for( ; i != table.StateMap.end(); ++i ) {

			if( i->second.RecordId == recordId )
				break;	// found
		}

		// check if not found...
		if( i == table.StateMap.end() ) {
			// register a DELETE, no data
			trace.log("found DELETE change");

			OSyncChange *change = osync_change_new();
			osync_change_set_changetype(change, CHANGE_DELETED);
			osync_change_set_member(change, env->member);
			osync_change_set_objformat_string(change, FormatName);

			char *uid = g_strdup_printf("%u", recordId);
			osync_change_set_uid(change, uid);
			trace.log(uid);
			g_free(uid);

			// report the change
			osync_context_report_change(ctx, change);
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
//
// OpenSync API
//

static void *initialize(OSyncMember *member, OSyncError **error)
{
	Trace trace("initialize");

	BarryEnvironment *env = 0;

	// Create the environment struct, including our Barry objects
	try {
		Barry::Init(true);

		env = new BarryEnvironment(member);

		// Load config file for this plugin
		char *configdata;
		int configsize;
		if (!osync_member_get_config(member, &configdata, &configsize, error)) {
			osync_error_update(error, "Unable to get config data: %s",
				osync_error_print(error));
			delete env;
			return NULL;
		}

		// Process the configdata here and set the options on your environment
		env->ParseConfig(configdata, configsize);
		free(configdata);

		// Load all needed cache files
		if( env->m_SyncCalendar ) {
			env->LoadCalendarCache();
		}

		if( env->m_SyncContacts ) {
			env->LoadContactsCache();
		}

		return env;

	}
	// Don't let C++ exceptions escape to the C code
	catch( std::bad_alloc &ba ) {
		osync_error_update(error, "Unable to allocate memory for environment: %s", ba.what());
		delete env;
		return NULL;
	}
	catch( std::exception &e ) {
		osync_error_update(error, "%s", e.what());
		delete env;
		return NULL;
	}
}

static void connect(OSyncContext *ctx)
{
	Trace trace("connect");

	try {

		// Each time you get passed a context (which is used to track
		// calls to your plugin) you can get the data your returned in
		// initialize via this call:
		BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

		// Probe for available devices
		Barry::Probe probe;
		int nIndex = probe.FindActive(env->m_pin);
		if( nIndex == -1 ) {
			osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Unable to find PIN %lu", env->m_pin);
			return;
		}
		env->m_ProbeResult = probe.Get(nIndex);

		// Create controller
		env->m_pCon = new Barry::Controller(env->m_ProbeResult);
		env->m_pCon->OpenMode(Barry::Controller::Desktop);

		// Success!
		osync_context_report_success(ctx);

	}
	// Don't let exceptions escape to the C modules
	catch( std::bad_alloc &ba ) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION,
			"Unable to allocate memory for controller: %s", ba.what());
	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION,
			"%s", e.what());
	}
}

static void get_changeinfo(OSyncContext *ctx)
{
	Trace trace("get_changeinfo");

	try {

		BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

		if( env->m_SyncCalendar ) {
			GetChanges(ctx, env, env->m_CalendarCache,
				"Calendar", "vevent20",
				&VEventConverter::GetRecordData);
		}

		if( env->m_SyncContacts ) {
			// FIXME - not yet implemented
//			GetChanges(ctx, env, env->m_ContactsCache,
//				"Address Book", "vcard30",
//				&ContactToVCard::GetRecordData);
		}

		// Success!
		osync_context_report_success(ctx);
	}
	// don't let exceptions escape to the C modules
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}

static bool commit_vevent20(BarryEnvironment *env,
			    unsigned int dbId,
			    OSyncContext *ctx,
			    OSyncChange *change)
{
	Trace trace("commit_vevent20");

	try {

		Barry::Controller &con = *env->m_pCon;
		Barry::RecordStateTable &table = env->m_CalendarTable;

		// extract RecordId from change's UID
		// FIXME - this may need some fudging, since there is no
		// guarantee that the UID will be a plain number
		const char *uid = osync_change_get_uid(change);
		unsigned long RecordId;
		if( sscanf(uid, "%lu", &RecordId) == 0 ) {
			osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER,
				"uid is not an expected number: %s", uid);
			return false;
		}

		// search for the RecordId in the state table, to find the
		// index... we only need the index if we are deleting or
		// modifying
		Barry::RecordStateTable::IndexType StateIndex;
		if( osync_change_get_changetype(change) != CHANGE_ADDED ) {
			if( !table.GetIndex(RecordId, &StateIndex) ) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
					"unable to get state table index for RecordId: %lu",
					RecordId);
				return false;
			}
		}

		switch( osync_change_get_changetype(change) )
		{
			case CHANGE_DELETED:
				con.DeleteRecord(dbId, StateIndex);
				break;

			case CHANGE_ADDED: {
				uint32_t newRecordId = env->m_CalendarTable.MakeNewRecordId();
				VEventConverter convert(newRecordId);
				if( !convert.ParseData(osync_change_get_data(change)) ) {
					osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "unable to parse change data for new RecordId: %lu", newRecordId);
					return false;
				}

				Barry::RecordBuilder<Barry::Calendar, VEventConverter> builder(convert);
				con.AddRecord(dbId, builder);
				break;
				}

			case CHANGE_MODIFIED: {
				VEventConverter convert(RecordId);
				if( !convert.ParseData(osync_change_get_data(change)) ) {
					osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "unable to parse change data for RecordId: %lu", RecordId);
					return false;
				}

				Barry::RecordBuilder<Barry::Calendar, VEventConverter> builder(convert);
				con.SetRecord(dbId, StateIndex, builder);
				break;
				}

			default:
				osync_debug("barry-sync", 0, "Unknown change type");
				break;
		}

		// Answer the call
		osync_context_report_success(ctx);
		return true;

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
		return false;
	}
}

// this function will be called exactly once with all objects to write
// gathered in an array
static void batch_commit_vevent20(OSyncContext *ctx,
				  OSyncContext **contexts,
				  OSyncChange **changes)
{
	Trace trace("batch_commit_vevent20");

	BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);
	
	try {
		Barry::Controller &con = *env->m_pCon;

		// fetch state table
		unsigned int dbId = con.GetDBID("Calendar");
		con.GetRecordStateTable(dbId, env->m_CalendarTable);

		//
		// Cycle through changes, looking for modifications first,
		// so that the state table will be stable (paranoia here,
		// it's worth checking whether this is necessary, by
		// trying to overwrite, delete, add, then overwrite
		// another using the same table index... will it work?)
		//
		// Update: tests confirm that it does work, for
		// read/delete/read.
		//
		for( int i = 0; contexts[i] && changes[i]; i++ ) {
			if( osync_change_get_changetype(changes[i]) != CHANGE_ADDED ) {
				if( !commit_vevent20(env, dbId, contexts[i], changes[i]) ) {
					osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
						"error committing context %i", i);
					return;
				}
			}
		}
		for( int i = 0; contexts[i] && changes[i]; i++ ) {
			if( osync_change_get_changetype(changes[i]) == CHANGE_ADDED ) {
				if( !commit_vevent20(env, dbId, contexts[i], changes[i]) ) {
					osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
						"error committing context %i", i);
					return;
				}
			}
		}

		// get the state table again, so we can update
		// the cache on success, later
		con.GetRecordStateTable(dbId, env->m_CalendarTable);

		// all done
		osync_context_report_success(ctx);

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}

static void sync_done(OSyncContext *ctx)
{
	Trace trace("sync_done");

//	BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

	//
	// This function will only be called if the sync was successfull
	//

// FIXME - finish this
	// update the caches
	// copy the latest record state table into the cache,
	// then write the cache to the file
//	env->m_CalendarTable, and env->m_CalendarCache

	// then do it again for the ContactsCache...

	// Success
	osync_context_report_success(ctx);
}

static void disconnect(OSyncContext *ctx)
{
	Trace trace("disconnect");

	// Disconnect the controller, which closes our connection
	BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);
	env->Disconnect();

	// Done!
	osync_context_report_success(ctx);
}

static void finalize(void *data)
{
	Trace trace("finalize");

	BarryEnvironment *env = (BarryEnvironment *)data;
	delete env;
}

void get_info(OSyncEnv *env)
{
	Trace trace("get_info");

	// Create first plugin
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	info->name = "barry-sync";
	info->longname = "Barry OpenSync plugin for the Blackberry handheld";
	info->description = "Plugin to synchronize calendar and contact entries on USB Blackberry handhelds";
	info->version = 1;		// API version (opensync api?)
	info->is_threadsafe = TRUE;
	
	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	
	// If you like, you can overwrite the default timeouts of your plugin
	// The default is set to 60 sec. Note that this MUST NOT be used to
	// wait for expected timeouts (Lets say while waiting for a webserver).
	// you should wait for the normal timeout and return a error.
//	info->timeouts.connect_timeout = 5;
	// There are more timeouts for the other functions

	//
	// Register each supported feature
	//

	// Calendar entries, using batch commit
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent20", NULL);
	osync_plugin_set_batch_commit_objformat(info, "event", "vevent20",
						batch_commit_vevent20);

	// Address Book entries
//	osync_plugin_accept_objtype(info, "contact");
//	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
//	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard30",
//						batch_commit_vcard30);

}

