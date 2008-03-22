//
// \file	barry_sync.cc
//		Opensync module for the USB Blackberry handheld
//

/*
    Copyright (C) 2006-2008, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <barry/barry.h>
#include <barry/dll.h>
#include "barry_sync.h"
#include "environment.h"
#include "vevent.h"
#include "vcard.h"
#include "trace.h"
#include <string>
#include <glib.h>
#include <stdint.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>

// All functions that are callable from outside must look like C
extern "C" {
	static void *initialize(OSyncMember *member, OSyncError **error);
	static void connect(OSyncContext *ctx);
	static void get_changeinfo(OSyncContext *ctx);
	static void sync_done(OSyncContext *ctx);
	static void disconnect(OSyncContext *ctx);
	static void finalize(void *data);
	BXEXPORT void get_info(OSyncEnv *env);
}


//////////////////////////////////////////////////////////////////////////////
//
// Support functions and classes
//


void GetChanges(OSyncContext *ctx, BarryEnvironment *env,
		DatabaseSyncState *pSync,
		const char *DBDBName,
		const char *ObjTypeName, const char *FormatName,
		GetData_t getdata)
{
	Trace trace("GetChanges");

	// shortcut references
	using namespace Barry;
	using Barry::RecordStateTable;
	Mode::Desktop &desktop = *env->m_pDesktop;

	// find the matching cache, state table, and id map for this change
	DatabaseSyncState::cache_type &cache = pSync->m_Cache;
	idmap &map = pSync->m_IdMap;

	// check if slow sync has been requested, and if so, empty the
	// cache and id map and start fresh
	if( osync_member_get_slow_sync(env->member, ObjTypeName) ) {
		trace.log("GetChanges: slow sync request detected, clearing cache and id map");
		cache.clear();
		map.clear();
	}

	// fetch state table
	unsigned int dbId = desktop.GetDBID(DBDBName);
	RecordStateTable &table = pSync->m_Table;
	desktop.GetRecordStateTable(dbId, table);

	// cycle through the state table...
	//    - if not in cache, it is added.
	//    - if in cache, check Blackberry's dirty flag
	RecordStateTable::StateMapType::const_iterator i = table.StateMap.begin();
	for( ; i != table.StateMap.end(); ++i ) {

		OSyncChange *change = 0;
		const RecordStateTable::IndexType &index = i->first;
		const RecordStateTable::State &state = i->second;

		// search the idmap for the UID
		std::string uid = pSync->Map2Uid(state.RecordId);

		// search the cache
		DatabaseSyncState::cache_type::const_iterator c = cache.find(state.RecordId);
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

			osync_change_set_uid(change, uid.c_str());
			trace.logf("change record ID: %s", uid.c_str());

			// Now you can set the data for the object
			// Set the last argument to FALSE if the real data
			// should be queried later in a "get_data" function
			char *data = (*getdata)(env, dbId, index);
			osync_change_set_data(change, data, strlen(data), TRUE);

			// just report the change via
			osync_context_report_change(ctx, change);

			// map our IDs for later
			map.Map(uid, state.RecordId);
		}
	}

	// now cycle through the cache... any objects in the cache
	// but not found in the state table means that they have been
	// deleted in the device
	DatabaseSyncState::cache_type::const_iterator c = cache.begin();
	for( ; c != cache.end(); ++c ) {
		uint32_t recordId = c->first;

		// search the idmap for the UID
		std::string uid = pSync->Map2Uid(recordId);

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

			osync_change_set_uid(change, uid.c_str());
			trace.log(uid.c_str());

			// report the change
			osync_context_report_change(ctx, change);
		}
	}

	// finally, cycle through the state map again, and overwrite the
	// cache with the current state table. Memory only... if successful,
	// it will be written back to disk later on.

	// start fresh
	cache.clear();

	for( i = table.StateMap.begin(); i != table.StateMap.end(); ++i ) {
		const RecordStateTable::State &state = i->second;
		cache[state.RecordId] = false;
	}
}

CommitData_t GetCommitFunction(OSyncChange *change)
{
	OSyncObjType *type = osync_change_get_objtype(change);
	const char *name = osync_objtype_get_name(type);
	if( strcmp(name, "event") == 0 ) {
		return &VEventConverter::CommitRecordData;
	}
	else if( strcmp(name, "contact") == 0 ) {
		return &VCardConverter::CommitRecordData;
	}
	else {
		return 0;
	}
}

bool FinishSync(OSyncContext *ctx, BarryEnvironment *env, DatabaseSyncState *pSync)
{
	Trace trace("FinishSync()");

	if( !pSync->m_Sync ) {
		// this mode is disabled in config, skip
		return true;
	}

	Barry::Mode::Desktop &desktop = *env->m_pDesktop;

	// get the state table again, so we can update
	// the cache properly
	desktop.GetRecordStateTable(pSync->m_dbId, pSync->m_Table);

	// update the cache
	if( !pSync->SaveCache() ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
			"Error saving calendar cache");
		return false;
	}

	// save the id map
	pSync->CleanupMap();
	if( !pSync->SaveMap() ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
			"Error saving calendar id map");
		return false;
	}

	// clear all dirty flags in device
	env->ClearDirtyFlags(pSync->m_Table, pSync->m_dbName);
	return true;
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

		// FIXME - near the end of release, do a run with
		// this set to true, and look for USB protocol
		// inefficiencies.
		Barry::Init(env->m_DebugMode);

		// Load all needed cache files
		if( env->m_CalendarSync.m_Sync ) {
			env->m_CalendarSync.LoadCache();
			env->m_CalendarSync.LoadMap();
		}

		if( env->m_ContactsSync.m_Sync ) {
			env->m_ContactsSync.LoadCache();
			env->m_ContactsSync.LoadMap();
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
			osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Unable to find PIN %lx", env->m_pin);
			return;
		}
		env->m_ProbeResult = probe.Get(nIndex);

		env->Connect(probe.Get(nIndex));

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

		if( env->m_CalendarSync.m_Sync ) {
			GetChanges(ctx, env, &env->m_CalendarSync,
				"Calendar", "event", "vevent20",
				&VEventConverter::GetRecordData);
		}

		if( env->m_ContactsSync.m_Sync ) {
			GetChanges(ctx, env, &env->m_ContactsSync,
				"Address Book", "contact", "vcard30",
				&VCardConverter::GetRecordData);
		}

		// Success!
		osync_context_report_success(ctx);
	}
	// don't let exceptions escape to the C modules
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}

static osync_bool commit_change(OSyncContext *ctx, OSyncChange *change)
{
	Trace trace("commit_change");

	// We can rely on a valid record state table, since get_changeinfo()
	// will be called first, and will fill the table.

	try {

		BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

		// find the needed commit function, based on objtype of the change
		CommitData_t CommitData = GetCommitFunction(change);
		if( !CommitData ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"unable to get commit function pointer");
			return false;
		}

		// find the matching cache, state table, and id map for this change
		DatabaseSyncState *pSync = env->GetSyncObject(change);
		if( !pSync ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"unable to get sync object that matches change type");
			return false;
		}

		// is syncing turned on for this type?
		if( !pSync->m_Sync ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"This object type is disabled in the barry-sync config");
			return false;
		}

		// make references instead of pointers
		DatabaseSyncState::cache_type &cache = pSync->m_Cache;
		Barry::RecordStateTable &table = pSync->m_Table;
		idmap &map = pSync->m_IdMap;
		Barry::Mode::Desktop &desktop = *env->m_pDesktop;
		unsigned int dbId = pSync->m_dbId;


		// extract RecordId from change's UID,
		// and update the ID map if necessary
		const char *uid = osync_change_get_uid(change);
		trace.logf("uid from change: %s", uid);
		if( strlen(uid) == 0 ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"uid from change object is blank!");
		}
		unsigned long RecordId = pSync->GetMappedRecordId(uid);

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

		std::string errmsg;
		bool status;

		switch( osync_change_get_changetype(change) )
		{
		case CHANGE_DELETED:
			desktop.DeleteRecord(dbId, StateIndex);
			cache.erase(RecordId);
			map.UnmapUid(uid);
			break;

		case CHANGE_ADDED:
			status = (*CommitData)(env, dbId, StateIndex, RecordId,
				osync_change_get_data(change), true, errmsg);
			if( !status ) {
				trace.logf("CommitData() for ADDED state returned false: %s", errmsg.c_str());
				osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
				map.UnmapUid(uid);
				return false;
			}
			cache[RecordId] = false;
			break;

		case CHANGE_MODIFIED:
			status = (*CommitData)(env, dbId, StateIndex, RecordId,
				osync_change_get_data(change), false, errmsg);
			if( !status ) {
				trace.logf("CommitData() for MODIFIED state returned false: %s", errmsg.c_str());
				osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
				map.UnmapUid(uid);
				return false;
			}
			break;

		default:
			trace.log("Unknown change type");
			osync_debug("barry-sync", 0, "Unknown change type");
			break;
		}

		// Answer the call
		osync_context_report_success(ctx);
		return true;


	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());

		// we don't worry about unmapping ids here, as there
		// is still a possibility that the record was added...
		// plus, the map might not get written out to disk anyway
		// in a plugin error state

		return false;
	}
}

static void sync_done(OSyncContext *ctx)
{
	//
	// This function will only be called if the sync was successfull
	//

	Trace trace("sync_done");

	try {

		BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

		// we reconnect to the device here, since dirty flags
		// for records we've just touched do not show up until
		// a disconnect... as far as I can tell.
		env->Reconnect();

		// do cleanup for each database
		if( FinishSync(ctx, env, &env->m_CalendarSync) &&
		    FinishSync(ctx, env, &env->m_ContactsSync) )
		{
			// Success
			osync_context_report_success(ctx);
		}

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
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
	info->longname = "Barry OpenSync plugin v0.13 for the Blackberry handheld";
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
	osync_plugin_set_commit_objformat(info, "event", "vevent20",
						commit_change);

	// Address Book entries
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30",
						commit_change);

}

