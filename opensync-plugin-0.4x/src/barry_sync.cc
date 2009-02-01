//
// \file	barry_sync.cc
//		Opensync module for the USB Blackberry handheld
//

/*
	Copyright (C) 2009, Nicolas VIVIEN (opensync plugin porting on opensync 0.4x)
    Copyright (C) 2006-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

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
#include <string.h>

// All functions that are callable from outside must look like C
extern "C" {
	BXEXPORT int get_version(void);
	static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);
	static osync_bool discover(OSyncPluginInfo *info, void *userdata, OSyncError **error);
	static void connect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);
	static void get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata);
	static void commit_change(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change, void *userdata);
	static void sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);
	static void disconnect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);
	static void finalize(void *userdata);
	BXEXPORT osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error);
}



//////////////////////////////////////////////////////////////////////////////
//
// Support functions and classes
//


void GetChanges(OSyncPluginInfo *info, OSyncContext *ctx, BarryEnvironment *env,
		DatabaseSyncState *pSync,
		const char *DBDBName,
		const char *ObjTypeName, const char *FormatName,
		GetData_t getdata)
{
	Trace trace("GetChanges");

	OSyncError *error = NULL;

	// shortcut references
	using namespace Barry;
	using Barry::RecordStateTable;
	Mode::Desktop &desktop = *env->m_pDesktop;

	// find the matching cache, state table, and id map for this change
	DatabaseSyncState::cache_type &cache = pSync->m_Cache;
	idmap &map = pSync->m_IdMap;

	// check if slow sync has been requested, and if so, empty the
	// cache and id map and start fresh
	if (osync_objtype_sink_get_slowsync(pSync->sink)) {
		trace.log("GetChanges: slow sync request detected, clearing cache and id map");
		cache.clear();
		map.clear();
	}

	// fetch state table
	unsigned int dbId = desktop.GetDBID(DBDBName);
	RecordStateTable &table = pSync->m_Table;
	desktop.GetRecordStateTable(dbId, table);

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

//	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);


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
			change = osync_change_new(&error);
			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
		}
		else {
			// in the cache... dirty?
			if( state.Dirty ) {
				// modified
				trace.log("found a MODIFIED change");
				change = osync_change_new(&error);
				osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);
			}
			else {
				trace.log("no change detected");
			}
		}

		// finish filling out the change object
		if( change ) {
//			osync_change_set_member(change, env->member);
//			osync_change_set_objformat_string(change, FormatName);

			osync_change_set_uid(change, uid.c_str());
			trace.logf("change record ID: %s", uid.c_str());

			OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, FormatName);

			// Now you can set the data for the object
			// Set the last argument to FALSE if the real data
			// should be queried later in a "get_data" function
			char *data = (*getdata)(env, dbId, index);
			OSyncData *odata = osync_data_new(data, strlen(data), format, &error);

//			osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

			osync_change_set_data(change, odata);
			osync_data_unref(odata);

			// just report the change via
			osync_context_report_change(ctx, change);

			osync_change_unref(change);

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

			OSyncChange *change = osync_change_new(&error);

			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

//			osync_change_set_member(change, env->member);
//			osync_change_set_objformat_string(change, FormatName);

			osync_change_set_uid(change, uid.c_str());
			trace.log(uid.c_str());

			OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, FormatName);

			OSyncData *odata = osync_data_new(NULL, 0, format, &error);

//			osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

			osync_change_set_data(change, odata);
			osync_data_unref(odata);

			// report the change
			osync_context_report_change(ctx, change);
	
			osync_change_unref(change);
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
	const char *name = osync_change_get_objtype(change);

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


static bool barry_contact_initialize(DatabaseSyncState *env, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("contact initialize");

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "contact");
	if (!sink)
		return false;
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
	if (!sinkEnabled)
		return false;

	trace.log("contact enabled");
	
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));

	functions.connect = connect;
	functions.disconnect = disconnect;
	functions.get_changes = get_changes;
	functions.commit = commit_change; 
	functions.sync_done = sync_done;

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "contact");

	OSyncList *objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);
	
	bool hasObjFormat = false;

	OSyncList *r;
	for(r = objformatsinks;r;r = r->next) {
		OSyncObjFormatSink *objformatsink = (OSyncObjFormatSink *) r->data;

		if(!strcmp("vcard30", osync_objformat_sink_get_objformat(objformatsink))) {
			trace.log("vcard30 found in barry-sync");
			hasObjFormat = true;
			break;
		}
	}
	
	if (!hasObjFormat) {
		return false;
	}

//	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

//	env->format = osync_format_env_find_objformat(formatenv, "vcard30");
	env->sink = sink;

	osync_objtype_sink_set_functions(sink, functions, NULL);

	trace.log("contact initialize OK");

	return true;
}


static bool barry_calendar_initialize(DatabaseSyncState *env, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("calendar initialize");

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "event");
	if (!sink)
		return false;
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
	if (!sinkEnabled)
		return false;
	
	trace.log("calendar enabled");

	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));

	functions.connect = connect;
	functions.disconnect = disconnect;
	functions.get_changes = get_changes;
	functions.commit = commit_change; 
	functions.sync_done = sync_done;

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "event");

	OSyncList *objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);
	
	bool hasObjFormat = false;

	OSyncList *r;
	for(r = objformatsinks;r;r = r->next) {
		OSyncObjFormatSink *objformatsink = (OSyncObjFormatSink *) r->data;

		if(!strcmp("vevent20", osync_objformat_sink_get_objformat(objformatsink))) {
			hasObjFormat = true;
			break;
		}
	}
	
	if (!hasObjFormat) {
		return false;
	}

//	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

//	env->format = osync_format_env_find_objformat(formatenv, "vevent20");
	env->sink = sink;

	osync_objtype_sink_set_functions(sink, functions, NULL);

	return true;
}




//////////////////////////////////////////////////////////////////////////////
//
// OpenSync API
//

static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("initialize");

	BarryEnvironment *env = 0;

	try {
		env = new BarryEnvironment(info);

		/*
		 * Read plugin config
		 */
		OSyncPluginConfig *config = osync_plugin_info_get_config(info);

		if (!config) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get config.");

			delete env;

			return NULL;
		}

		/*
		 * Process plugin specific advanced options 
		 */
		OSyncList *optslist = osync_plugin_config_get_advancedoptions(config);
		for (; optslist; optslist = optslist->next) {
			OSyncPluginAdvancedOption *option = (OSyncPluginAdvancedOption *) optslist->data;
	
			const char *val = osync_plugin_advancedoption_get_value(option);
			const char *name = osync_plugin_advancedoption_get_name(option);
	
			if (!strcmp(name, "PinCode")) {
				env->m_pin = atoi(val);
			}
			else if (!strcmp(name, "Debug")) {
				env->m_DebugMode = (!strcmp(val, "1")) ? true : false;
			}
		}

		OSyncPluginAuthentication *optauth = osync_plugin_config_get_authentication(config);

		if (osync_plugin_authentication_option_is_supported(optauth, OSYNC_PLUGIN_AUTHENTICATION_PASSWORD)) {
			const char *val = osync_plugin_authentication_get_password(optauth);

			env->m_password = val;
		}


		// FIXME - near the end of release, do a run with
		// this set to true, and look for USB protocol
		// inefficiencies.
		Barry::Init(env->m_DebugMode);


		/*
		 * Process Ressource options
		 */
		trace.log("Process Ressource options...");

		if (barry_calendar_initialize(&env->m_CalendarSync, info, error)) {
			env->m_CalendarSync.LoadCache();
			env->m_CalendarSync.LoadMap();

			env->m_CalendarSync.m_Sync = true;
		}		
		else {
			trace.log("No sync Calendar");

			env->m_CalendarSync.m_Sync = false;
		}

		if (barry_contact_initialize(&env->m_ContactsSync, info, error)) {
			env->m_ContactsSync.LoadCache();
			env->m_ContactsSync.LoadMap();

			env->m_ContactsSync.m_Sync = true;
		}		
		else {
			trace.log("No sync Contact");

			env->m_ContactsSync.m_Sync = false;
		}
	

		return (void *) env;
	}
	catch( std::exception &e ) {
		delete env;

		return NULL;
	}
}


static osync_bool discover(OSyncPluginInfo *info, void *userdata, OSyncError **error)
{
	Trace trace("discover");

	int i, numobjs = osync_plugin_info_num_objtypes(info);
    
	for (i = 0; i < numobjs; i++) {
    	OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(info, i);

		osync_objtype_sink_set_available(sink, true);
	}

	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "Barry");
	osync_version_set_modelversion(version, "1");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	return true;
}


static void connect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	Trace trace("connect");

	try {

		// Each time you get passed a context (which is used to track
		// calls to your plugin) you can get the data your returned in
		// initialize via this call:
		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// Probe for available devices
		Barry::Probe probe;
		int nIndex = probe.FindActive(env->m_pin);
		if( nIndex == -1 ) {
			osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Unable to find PIN %lx", env->m_pin);
			return;
		}
		env->m_ProbeResult = probe.Get(nIndex);

		trace.log("connecting...");

		env->Connect(probe.Get(nIndex));

		trace.log("connected !");

		// Success!
		osync_context_report_success(ctx);
	}
	// Don't let exceptions escape to the C modules
	catch( std::bad_alloc &ba ) {
		trace.log("bad alloc");

		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION,
			"Unable to allocate memory for controller: %s", ba.what());
	}
	catch( std::exception &e ) {
		trace.log("exception");

		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION,
			"%s", e.what());
	}

	trace.log("exit connect");
}

static void get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
	Trace trace("get_changeinfo");

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

		if( env->m_CalendarSync.m_Sync ) {
			GetChanges(info, ctx, env, &env->m_CalendarSync,
				"Calendar", "event", "vevent20",
				&VEventConverter::GetRecordData);
		}

		if( env->m_ContactsSync.m_Sync ) {
			GetChanges(info, ctx, env, &env->m_ContactsSync,
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

static void commit_change(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change, void *userdata)
{
	Trace trace("commit_change");

	// We can rely on a valid record state table, since get_changeinfo()
	// will be called first, and will fill the table.

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// find the needed commit function, based on objtype of the change
		CommitData_t CommitData = GetCommitFunction(change);
		if( !CommitData ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"unable to get commit function pointer");
			return;
		}

		// find the matching cache, state table, and id map for this change
		DatabaseSyncState *pSync = env->GetSyncObject(change);
		if( !pSync ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"unable to get sync object that matches change type");
			return;
		}

		// is syncing turned on for this type?
		if( !pSync->m_Sync ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
				"This object type is disabled in the barry-sync config");
			return;
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
		if( osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_ADDED ) {
			if( !table.GetIndex(RecordId, &StateIndex) ) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
					"unable to get state table index for RecordId: %lu",
					RecordId);
				return;
			}
		}

		std::string errmsg;
		bool status;

		OSyncData *odata = NULL;
		char *plain = NULL;

		switch( osync_change_get_changetype(change) )
		{
		case OSYNC_CHANGE_TYPE_DELETED:
			desktop.DeleteRecord(dbId, StateIndex);
			cache.erase(RecordId);
			map.UnmapUid(uid);
			break;

		case OSYNC_CHANGE_TYPE_ADDED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			status = (*CommitData)(env, dbId, StateIndex, RecordId,
				plain, true, errmsg);
			if( !status ) {
				trace.logf("CommitData() for ADDED state returned false: %s", errmsg.c_str());
				osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
				map.UnmapUid(uid);
				return;
			}
			cache[RecordId] = false;
			break;

		case OSYNC_CHANGE_TYPE_MODIFIED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			status = (*CommitData)(env, dbId, StateIndex, RecordId,
				plain, false, errmsg);
			if( !status ) {
				trace.logf("CommitData() for MODIFIED state returned false: %s", errmsg.c_str());
				osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
				map.UnmapUid(uid);
				return;
			}
			break;

		default:
			trace.log("Unknown change type");
			break;
		}

		// Answer the call
		osync_context_report_success(ctx);
		return;


	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());

		// we don't worry about unmapping ids here, as there
		// is still a possibility that the record was added...
		// plus, the map might not get written out to disk anyway
		// in a plugin error state

		return;
	}
}

static void sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	//
	// This function will only be called if the sync was successfull
	//

	Trace trace("sync_done");

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

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

static void disconnect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	Trace trace("disconnect");

	// Disconnect the controller, which closes our connection
	BarryEnvironment *env = (BarryEnvironment *) userdata;
	env->Disconnect();

	// Done!
	osync_context_report_success(ctx);
}

static void finalize(void *userdata)
{
	Trace trace("finalize");

	BarryEnvironment *env = (BarryEnvironment *) userdata;

	delete env;
}


osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	Trace trace("get_sync_info");

	// Create a new OpenSync plugin
	OSyncPlugin *plugin = osync_plugin_new(error);

	if (!plugin) {
		osync_error_unref(error);

		return false;
	}

	// Describe our plugin
	osync_plugin_set_name(plugin, "barry-sync");
	osync_plugin_set_longname(plugin, "Barry OpenSync plugin v0.15 for the Blackberry handheld");
	osync_plugin_set_description(plugin, "Plugin to synchronize calendar and contact entries on USB Blackberry handhelds");
	
	// Set the callback functions
	osync_plugin_set_initialize(plugin, initialize);
	osync_plugin_set_finalize(plugin, finalize);
	osync_plugin_set_discover(plugin, discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	return true;
}


int get_version(void)
{
	return 1;
}

