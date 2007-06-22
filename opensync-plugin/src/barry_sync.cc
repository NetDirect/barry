///
/// \file	barry_sync.cc
///		Opensync module for the USB Blackberry handheld
///
///		A lot of this code is based heavily on the example
///		plugin from the OpenSync project:
///		http://www.opensync.org/
///		OpenSync is distributed under LGPLv2.1
///

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

#include <opensync/opensync.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-version.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <barry/barry.h>
#include "barry_sync.h"
#include "environment.h"
#include "vevent.h"
#include "trace.h"
#include <string>
#include <glib.h>
#include <stdint.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>

// All functions that are callable from outside must look like C
extern "C" {
	osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error);
	int get_version(void);
}


//////////////////////////////////////////////////////////////////////////////
//
// Support functions and classes
//

void GetChanges(OSyncContext *ctx, OSyncObjTypeSink *sink,
		BarryEnvironment *env, DatabaseSyncState *sync,
		GetData_t getdata)
{
	Trace trace("GetChanges", sync->GetDesc().c_str());

	OSyncError *error = NULL;

	// shortcut references
	using namespace Barry;
	using Barry::RecordStateTable;
	Controller &con = *env->m_pCon;
	DatabaseSyncState::cache_type &cache = sync->m_Cache;

	// fetch state table
	unsigned int dbId = con.GetDBID(sync->GetDBName());
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
		DatabaseSyncState::cache_type::const_iterator c = cache.find(state.RecordId);
		if( c == cache.end() ) {
			// not in cache, this is a new item
			trace.log("found an ADDED change");
			change = osync_change_new(&error);
			if( !change ) {
				trace.error("osync_change_new() failed");
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				continue;
			}
			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
		}
		else {
			// in the cache... dirty?
			if( state.Dirty ) {
				// modified
				trace.log("found a MODIFIED change");
				change = osync_change_new(&error);
				if( !change ) {
					trace.error("osync_change_new() failed");
					osync_context_report_osyncwarning(ctx, error);
					osync_error_unref(&error);
					continue;
				}
				osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);
			}
			else {
				trace.log("no change detected");
			}
		}

		// finish filling out the change object
		if( change ) {
			char *uid = g_strdup_printf("%u", state.RecordId);
			osync_change_set_uid(change, uid);
			trace.logf("change record ID: %s", uid);
			g_free(uid);

			// Now you can set the data for the object
			// Set the last argument to FALSE if the real data
			// should be queried later in a "get_data" function
			char *data = (*getdata)(env, dbId, index);

			OSyncData *odata = osync_data_new(NULL, 0, sync->m_pObjFormat, &error);
			if( !odata ) {
				osync_change_unref(change);
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				continue;
			}

			osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
			osync_data_set_data(odata, data, strlen(data));

			osync_change_set_data(change, odata);
			osync_data_unref(odata);

			// just report the change via
			osync_context_report_change(ctx, change);
		}
	}

	// now cycle through the cache... any objects in the cache
	// but not found in the state table means that they have been
	// deleted in the device
	DatabaseSyncState::cache_type::const_iterator c = cache.begin();
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

			OSyncChange *change = osync_change_new(&error);
			if( !change ) {
				trace.error("osync_change_new() failed");
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				continue;
			}

			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

			OSyncData *odata = osync_data_new(NULL, 0, sync->m_pObjFormat, &error);
			if( !odata ) {
				osync_change_unref(change);
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
				continue;
			}
			osync_change_set_data(change, odata);

			osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

			char *uid = g_strdup_printf("%u", recordId);
			osync_change_set_uid(change, uid);
			trace.log(uid);
			g_free(uid);

			osync_change_set_data(change, odata);
			osync_data_unref(odata);

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

/*
CommitData_t GetCommitFunction(OSyncChange *change)
{
	OSyncObjType *type = osync_change_get_objtype(change);
	const char *name = osync_objtype_get_name(type);
	if( strcmp(name, "event") == 0 ) {
		return &VEventConverter::CommitRecordData;
	}
	else if( strcmp(name, "contact") == 0 ) {
		return 0;
//		return &VCardConverter::CommitRecordData;
	}
	else {
		return 0;
	}
}


bool CommitChange(BarryEnvironment *env,
		  unsigned int dbId,
		  OSyncContext *ctx,
		  OSyncChange *change)
{
	Trace trace("CommitChange");

	try {

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

		// make references instead of pointers
		DatabaseSyncState::cache_type &cache = pSync->m_Cache;
		Barry::RecordStateTable &table = pSync->m_Table;
		idmap &map = pSync->m_IdMap;
		Barry::Controller &con = *env->m_pCon;


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

		switch( osync_change_get_changetype(change) )
		{
			case CHANGE_DELETED:
				con.DeleteRecord(dbId, StateIndex);
				cache.erase(RecordId);
				map.UnmapUid(uid);
				break;

			case CHANGE_ADDED:
				if( !(*CommitData)(env, dbId, StateIndex, RecordId, osync_change_get_data(change), true, errmsg) ) {
					trace.logf("CommitData() for ADDED state returned false: %s", errmsg.c_str());
					osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
					map.UnmapUid(uid);
					return false;
				}
				cache[RecordId] = false;
				break;

			case CHANGE_MODIFIED:
				if( !(*CommitData)(env, dbId, StateIndex, RecordId, osync_change_get_data(change), false, errmsg) ) {
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
*/




//////////////////////////////////////////////////////////////////////////////
//
// OpenSync sink callbacks
//

static void connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	Trace trace("connect");

	try {

		// Each time you get passed a context (which is used to track
		// calls to your plugin) you can get the data your returned in
		// initialize via this call:
		BarryEnvironment *env = (BarryEnvironment *)userdata;

		if( env->IsConnected() ) {
			// we've been here before.... possibly another "sink"
			// was run ahead of us
			trace.log("Already connected in connect()");
			return;
		}

		// Probe for available devices
		Barry::Probe probe;
		int nIndex = probe.FindActive(env->m_pin);
		if( nIndex == -1 ) {
			osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Unable to find PIN %lu", env->m_pin);
			return;
		}
		env->m_ProbeResult = probe.Get(nIndex);

		// Create controller
		env->OpenDesktop(env->m_ProbeResult);

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

static void get_changes(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	Trace trace("get_changes");

	BarryEnvironment *env = (BarryEnvironment*) userdata;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	DatabaseSyncState *sync = (DatabaseSyncState*) osync_objtype_sink_get_userdata(sink);

	if( !sync ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
			"Unable to retrieve database sync state.");
		return;
	}

	trace.logf("get_chagnes for %s", sync->GetDesc().c_str());

	try {

		GetChanges(ctx, sink, env, sync,
			&VEventConverter::GetRecordData); // FIXME ack, hard coded

		// Success!
		osync_context_report_success(ctx);
	}
	// don't let exceptions escape to the C modules
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}

// this function will be called exactly once with all objects to write
// gathered in an array
static void batch_commit_calendar(void *userdata,
				  OSyncPluginInfo *info,
				  OSyncContext *ctx,
				  OSyncContext **contexts,
				  OSyncChange **changes)
{
/*
	Trace trace("batch_commit_vevent20");

	BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);
	
	try {
		Barry::Controller &con = *env->m_pCon;

		// fetch state table
		unsigned int dbId = con.GetDBID("Calendar");
		con.GetRecordStateTable(dbId, env->m_CalendarSync.m_Table);

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
				if( !CommitChange(env, dbId, contexts[i], changes[i]) ) {
					osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
						"error committing context %i", i);
					return;
				}
			}
		}
		for( int i = 0; contexts[i] && changes[i]; i++ ) {
			if( osync_change_get_changetype(changes[i]) == CHANGE_ADDED ) {
				if( !CommitChange(env, dbId, contexts[i], changes[i]) ) {
					osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
						"error committing context %i", i);
					return;
				}
			}
		}

		// get the state table again, so we can update
		// the cache on success, later
		con.GetRecordStateTable(dbId, env->m_CalendarSync.m_Table);

		// all done
		osync_context_report_success(ctx);

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
*/
}

static void sync_done(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	//
	// This function will only be called if the sync was successful
	//

	Trace trace("sync_done");

//	BarryEnvironment *env = (BarryEnvironment*) userdata;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	DatabaseSyncState *sync = (DatabaseSyncState*) osync_objtype_sink_get_userdata(sink);
	bool error = false;

	if( !sync ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
			"Unable to retrieve database sync state.");
		return;
	}

	trace.logf("sync_done for %s", sync->GetDesc().c_str());

	try {


		// update the cache
		if( !sync->SaveCache() ) {
			error = true;
			trace.errorf("Error saving %s cache", sync->GetDesc().c_str());
			osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
				trace.get_last_msg());
		}

		// save the id map
		sync->CleanupMap();
		if( !sync->SaveMap() ) {
			error = true;
			trace.errorf("Error saving %s id map", sync->GetDesc().c_str());
			osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
				trace.get_last_msg());
		}

		// clear all dirty flags
		sync->ClearDirtyFlags();

		// Success
		if( !error )
			osync_context_report_success(ctx);

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}

static void disconnect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	Trace trace("disconnect");

	// Disconnect the controller, which closes our connection
	BarryEnvironment *env = (BarryEnvironment *)userdata;
	env->Disconnect();

	// Done!
	osync_context_report_success(ctx);
}


//////////////////////////////////////////////////////////////////////////////
//
// OpenSync plugin callbacks
//

static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("initialize");


	// Create the environment struct, including our Barry objects
	try {
		// Load config file for this plugin
		const char *configdata = osync_plugin_info_get_config(info);
		if( !configdata ) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get config data.");
			return NULL;
		}
		osync_trace(TRACE_INTERNAL, "The config: %s", configdata);

		// FIXME - near the end of release, do a run with
		// this set to true, and look for USB protocol
		// inefficiencies.
		Barry::Init(false);

		std::auto_ptr<BarryEnvironment> env(new BarryEnvironment(info));

		// Process the configdata here and set the options on your environment
		env->ParseConfig(configdata);

		// Load all needed cache files
		env->m_CalendarSync.LoadCache();
		env->m_CalendarSync.LoadMap();

		env->m_ContactsSync.LoadCache();
		env->m_ContactsSync.LoadMap();

		// FIXME - this is needed down the road, in change processing,
		// and it appears to be sink-related... there's a lot we could
		// do here to organize this into classes, per-sink, etc.
		OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
		env->m_CalendarSync.m_pObjFormat =
			osync_format_env_find_objformat(formatenv, "vevent20");

		// Setup the Calendar sink, using batch commit
		OSyncObjTypeSink *sink = osync_objtype_sink_new("event", error);
		if( !sink ) {
			trace.error("Unable to create new sink.");
			return NULL;
		}

		osync_objtype_sink_add_objformat(sink, "xmlformat-event");

		// Every sink can have different functions
		OSyncObjTypeSinkFunctions functions;
		memset(&functions, 0, sizeof(functions));
		functions.connect = connect;
		functions.disconnect = disconnect;
		functions.get_changes = get_changes;
		functions.batch_commit = batch_commit_calendar;
		functions.sync_done = sync_done;

		// store pointer to database object in sink,
		// for functions to use later
		osync_objtype_sink_set_functions(sink, functions, &env->m_CalendarSync);

		// add and save sink pointer
		osync_plugin_info_add_objtype(info, sink);
		env->m_CalendarSync.m_pSink = sink;

		// FIXME - add sink for contacts too;
		// Address Book entries
//	osync_plugin_accept_objtype(info, "contact");
//	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
//	osync_plugin_set_batch_commit_objformat(info, "contact", "vcard30",
//						batch_commit_vcard30);


		// return our user data
		return env.release();

	}
	// Don't let C++ exceptions escape to the C code
	catch( std::bad_alloc &ba ) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to allocate memory for environment: %s", ba.what());
		return NULL;
	}
	catch( std::exception &e ) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", e.what());
		return NULL;
	}
}

static void finalize(void *userdata)
{
	Trace trace("finalize");

	BarryEnvironment *env = (BarryEnvironment *)userdata;
	delete env;
}

// Tell opensync which sinks are available
static osync_bool discover(void *userdata, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("discover");

	BarryEnvironment *env = (BarryEnvironment *)userdata;

	// Report avaliable sinks...
	osync_objtype_sink_set_available(env->m_CalendarSync.m_pSink, TRUE);

	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "barry-sync");
	//osync_version_set_modelversion(version, "version");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// OpenSync module API
//

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	Trace trace("get_sync_info");

	// Create first plugin
	OSyncPlugin *plugin = osync_plugin_new(error);
	if( !plugin ) {
		trace.errorf("Unable to register plugin: %s", osync_error_print(error));
		osync_error_unref(error);
		return FALSE;
	}

	// Boy, doesn't this look like a C++ API in disguise?.... :-)
	osync_plugin_set_name(plugin, "barry-sync");
	osync_plugin_set_longname(plugin, "Barry OpenSync plugin for the Blackberry handheld");
	osync_plugin_set_description(plugin, "Plugin to synchronize calendar and contact entries on USB Blackberry handhelds");

	// Register callback functions
	osync_plugin_set_initialize(plugin, initialize);
	osync_plugin_set_finalize(plugin, finalize);
	osync_plugin_set_discover(plugin, discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	return TRUE;
}

int get_version(void)
{
	Trace trace("get_version");
	return 1;
}

