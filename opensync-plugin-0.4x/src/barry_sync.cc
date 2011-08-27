//
// \file	barry_sync.cc
//		Opensync module for the USB Blackberry handheld
//

/*
	Copyright (C) 2009, Nicolas VIVIEN (opensync plugin porting on opensync 0.4x ; Task & Memo support)
    Copyright (C) 2006-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

#include <barry/barry.h>
#include <barry/dll.h>
#include "barry_sync.h"
#include "environment.h"
#include "vtodo.h"
#include "vjournal.h"
#include "vevent.h"
#include "vcard.h"
#include "trace.h"
#include <string>
#include <glib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

typedef Barry::vSmartPtr<OSyncList, OSyncList, &osync_list_free> AutoOSyncList;

// All functions that are callable from outside must look like C
extern "C" {
	BXEXPORT int get_version(void);
	static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);
	static osync_bool discover(OSyncPluginInfo *info, void *userdata, OSyncError **error);

	static void contact_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata);
	static void contact_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);

	static void event_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata);
	static void event_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);

	static void journal_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata);
	static void journal_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);

	static void todo_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata);
	static void todo_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);

	static void connect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);
	static void disconnect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata);
	static void commit_change(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change, void *userdata);
	static void finalize(void *userdata);
	BXEXPORT osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error);
}



//////////////////////////////////////////////////////////////////////////////
//
// Support functions and classes
//

// Generates a "hash" by grabbing the existing hash
// of the given uid, and adding 1 to it if dirty.
std::string GenerateHash(OSyncHashTable *hashtable,
			const std::string &uid,
			bool dirty)
{
	unsigned long hashcount = 0;

	const char *hash = osync_hashtable_get_hash(hashtable, uid.c_str());
	if( hash ) {
		errno = 0;
		hashcount = strtoul(hash, NULL, 10);
		if( errno )
			throw std::runtime_error("Error converting string to unsigned long: " + std::string(hash));
	}

	hashcount += (dirty ? 1 : 0);

	std::ostringstream oss;
	oss << std::dec << hashcount;
	return oss.str();
}

void GetChanges(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx,
		BarryEnvironment *env,
		DatabaseSyncState *pSync,
		const char *DBDBName,
		const char *ObjTypeName, const char *FormatName,
		GetData_t getdata,
		osync_bool slow_sync)
{
	Trace trace("GetChanges");

	OSyncError *error = NULL;

	// shortcut references
	using namespace Barry;
	using Barry::RecordStateTable;
	Mode::Desktop &desktop = *env->GetDesktop();

	// find hash table
	//
	// Note: Since the Blackberry tracks dirty flags for us, we don't
	//       need the hash table to help determine what records have
	//       changed, and therefore we don't need to actually load
	//       all the record data across USB either.
	//
	//       The hashtable only needs the hash to change when data
	//       has changed, so we set each change object's hash to:
	//             last_hash + (dirty ? 1 : 0)
	//
	OSyncHashTable *hashtable = osync_objtype_sink_get_hashtable(sink);

	// check if slow sync has been requested
	if (slow_sync) {
		trace.log("GetChanges: slow sync request detected");

		if( !osync_hashtable_slowsync(hashtable, &error) ) {
			std::ostringstream oss;
			oss << "GetChanges: slow sync error: " << osync_error_print(&error);
			osync_error_unref(&error);

			trace.log(oss.str().c_str());
			throw std::runtime_error(oss.str());
		}
	}

	// fetch state table
	unsigned int dbId = desktop.GetDBID(DBDBName);
	RecordStateTable &table = pSync->m_Table;
	desktop.GetRecordStateTable(dbId, table);

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);


	// cycle through the state table... for each record in the state
	// table, register a change and the fake hash (see note above)
	// and let the hash table determine what changetype it is
	RecordStateTable::StateMapType::const_iterator i = table.StateMap.begin();
	for( ; i != table.StateMap.end(); ++i ) {

		OSyncChange *change = 0;
		const RecordStateTable::IndexType &index = i->first;
		const RecordStateTable::State &state = i->second;

		// create change to pass to hashtable
		change = osync_change_new(&error);
		if( !change ) {
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		// convert record ID to uid string
		std::string uid = pSync->Map2Uid(state.RecordId);

		// setup change, just enough for hashtable use
		osync_change_set_uid(change, uid.c_str());
		trace.logf("change record ID: %s", uid.c_str());
		std::string hash = GenerateHash(hashtable, uid, state.Dirty);
		osync_change_set_hash(change, hash.c_str());


		// let hashtable determine what's going to happen
		OSyncChangeType changetype = osync_hashtable_get_changetype(hashtable, change);
		osync_change_set_changetype(change, changetype);

		// let hashtable know we've processed this change
		osync_hashtable_update_change(hashtable, change);


		// Decision time: if nothing has changed, skip
		if( changetype == OSYNC_CHANGE_TYPE_UNMODIFIED ) {
			osync_change_unref(change);
			continue;
		}


		//
		// finish filling out the change object
		//

		// Now you can set the data for the object
		// Set the last argument to FALSE if the real data
		// should be queried later in a "get_data" function
		OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, FormatName);
		char *data = (*getdata)(env, dbId, index);
		OSyncData *odata = osync_data_new(data, strlen(data), format, &error);

		if (!odata) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

// FIXME ? Is this line is usefull ?
//		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		// just report the change via
		osync_context_report_change(ctx, change);

		osync_change_unref(change);
	}

	// the hashtable can now give us a linked list of deleted
	// entries, after the above processing
	AutoOSyncList uids = osync_hashtable_get_deleted(hashtable);
	for( OSyncList *u = uids.Get(); u; u = u->next) {

		const char *uid = (const char*) u->data;
		uint32_t recordId = strtoul(uid, NULL, 10);

		// search the state table
		i = table.StateMap.begin();
		for( ; i != table.StateMap.end(); ++i ) {

			if( i->second.RecordId == recordId ) {
				throw std::runtime_error("Found deleted record ID in state map! " + std::string(uid));
			}
		}

		// register a DELETE, no data
		trace.log("found DELETE change");

		OSyncChange *change = osync_change_new(&error);
		if( !change ) {
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_change_set_uid(change, uid);
		trace.log(uid);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

//		osync_change_set_objformat_string(change, FormatName);

		OSyncObjFormat *format = osync_format_env_find_objformat(formatenv, FormatName);
		OSyncData *odata = osync_data_new(NULL, 0, format, &error);
		if( !odata ) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		// report the change
		osync_context_report_change(ctx, change);

		// tell hashtable we've processed this item
		osync_hashtable_update_change(hashtable, change);

		osync_change_unref(change);
	}
}

CommitData_t GetCommitFunction(OSyncChange *change)
{
	Trace trace("GetCommitFunction()");

	const char *name = osync_change_get_objtype(change);

	if( strcmp(name, "event") == 0 ) {
		return &VEventConverter::CommitRecordData;
	}
	else if( strcmp(name, "contact") == 0 ) {
		return &VCardConverter::CommitRecordData;
	}
	else if( strcmp(name, "note") == 0 ) {
		return &VJournalConverter::CommitRecordData;
	}
	else if( strcmp(name, "todo") == 0 ) {
		return &VTodoConverter::CommitRecordData;
	}
	else {
		trace.log("unknown !");
		trace.log(name);

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

	// we reconnect to the device here, since dirty flags
	// for records we've just touched do not show up until
	// a disconnect... as far as I can tell.
	env->ReconnectForDirtyFlags();

	// get the state table again, so we can update
	// the cache properly
	Barry::Mode::Desktop &desktop = *env->GetDesktop();
	desktop.GetRecordStateTable(pSync->m_dbId, pSync->m_Table);

	// clear all dirty flags in device
	env->ClearDirtyFlags(pSync->m_Table, pSync->m_dbName);
	return true;
}


static bool barry_contact_initialize(BarryEnvironment *env, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("contact initialize");

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "contact");
	if (!sink)
		return false;
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
	if (!sinkEnabled)
		return false;

	trace.log("contact enabled");

	osync_objtype_sink_set_connect_func(sink, connect);
	osync_objtype_sink_set_disconnect_func(sink, disconnect);
	osync_objtype_sink_set_get_changes_func(sink, contact_get_changes);
	osync_objtype_sink_set_commit_func(sink, commit_change);
	osync_objtype_sink_set_sync_done_func(sink, contact_sync_done);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "contact");

	AutoOSyncList objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);

	bool hasObjFormat = false;

	OSyncList *r;
	for(r = objformatsinks.Get();r;r = r->next) {
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
	env->m_ContactsSync.sink = sink;

	osync_objtype_sink_set_userdata(sink, env);

	osync_objtype_sink_enable_hashtable(sink, TRUE);

	trace.log("contact initialize OK");

	return true;
}


static bool barry_calendar_initialize(BarryEnvironment *env, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("calendar initialize");

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "event");
	if (!sink)
		return false;
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
	if (!sinkEnabled)
		return false;

	trace.log("calendar enabled");

	osync_objtype_sink_set_connect_func(sink, connect);
	osync_objtype_sink_set_disconnect_func(sink, disconnect);
	osync_objtype_sink_set_get_changes_func(sink, event_get_changes);
	osync_objtype_sink_set_commit_func(sink, commit_change);
	osync_objtype_sink_set_sync_done_func(sink, event_sync_done);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "event");

	AutoOSyncList objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);

	bool hasObjFormat = false;

	OSyncList *r;
	for(r = objformatsinks.Get();r;r = r->next) {
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
	env->m_CalendarSync.sink = sink;

	osync_objtype_sink_set_userdata(sink, env);

	osync_objtype_sink_enable_hashtable(sink, TRUE);

	return true;
}


static bool barry_journal_initialize(BarryEnvironment *env, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("journal initialize");

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "note");
	if (!sink)
		return false;
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
	if (!sinkEnabled)
		return false;

	trace.log("journal enabled");

	osync_objtype_sink_set_connect_func(sink, connect);
	osync_objtype_sink_set_disconnect_func(sink, disconnect);
	osync_objtype_sink_set_get_changes_func(sink, journal_get_changes);
	osync_objtype_sink_set_commit_func(sink, commit_change);
	osync_objtype_sink_set_sync_done_func(sink, journal_sync_done);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "note");

	AutoOSyncList objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);

	bool hasObjFormat = false;

	OSyncList *r;
	for(r = objformatsinks.Get();r;r = r->next) {
		OSyncObjFormatSink *objformatsink = (OSyncObjFormatSink *) r->data;

		if(!strcmp("vjournal", osync_objformat_sink_get_objformat(objformatsink))) {
			hasObjFormat = true;
			break;
		}
	}

	if (!hasObjFormat) {
		return false;
	}

//	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

//	env->format = osync_format_env_find_objformat(formatenv, "vjournal");
	env->m_JournalSync.sink = sink;

	osync_objtype_sink_set_userdata(sink, env);

	osync_objtype_sink_enable_hashtable(sink, TRUE);

	return true;
}


static bool barry_todo_initialize(BarryEnvironment *env, OSyncPluginInfo *info, OSyncError **error)
{
	Trace trace("todo initialize");

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "todo");
	if (!sink)
		return false;
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
	if (!sinkEnabled)
		return false;

	trace.log("todo enabled");

	osync_objtype_sink_set_connect_func(sink, connect);
	osync_objtype_sink_set_disconnect_func(sink, disconnect);
	osync_objtype_sink_set_get_changes_func(sink, todo_get_changes);
	osync_objtype_sink_set_commit_func(sink, commit_change);
	osync_objtype_sink_set_sync_done_func(sink, todo_sync_done);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "todo");

	AutoOSyncList objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);

	bool hasObjFormat = false;

	OSyncList *r;
	for(r = objformatsinks.Get();r;r = r->next) {
		OSyncObjFormatSink *objformatsink = (OSyncObjFormatSink *) r->data;

		if(!strcmp("vtodo20", osync_objformat_sink_get_objformat(objformatsink))) {
			hasObjFormat = true;
			break;
		}
	}

	if (!hasObjFormat) {
		return false;
	}

//	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

//	env->format = osync_format_env_find_objformat(formatenv, "vtodo20");
	env->m_TodoSync.sink = sink;

	osync_objtype_sink_set_userdata(sink, env);

	osync_objtype_sink_enable_hashtable(sink, TRUE);

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
		AutoOSyncList optslist = osync_plugin_config_get_advancedoptions(config);
		for (OSyncList *o = optslist.Get(); o; o = o->next) {
			OSyncPluginAdvancedOption *option = (OSyncPluginAdvancedOption *) o->data;

			const char *val = osync_plugin_advancedoption_get_value(option);
			const char *name = osync_plugin_advancedoption_get_name(option);

			if (!strcmp(name, "PinCode")) {
				env->m_pin = strtol(val, NULL, 16);
			}
			else if (!strcmp(name, "Debug")) {
				env->m_DebugMode = (!strcmp(val, "1")) ? true : false;
			}
		}

		OSyncPluginAuthentication *optauth = osync_plugin_config_get_authentication(config);

		if (osync_plugin_authentication_option_is_supported(optauth, OSYNC_PLUGIN_AUTHENTICATION_PASSWORD)) {
			const char *val = osync_plugin_authentication_get_password(optauth);

			env->SetPassword(val);
		}


		// FIXME - near the end of release, do a run with
		// this set to true, and look for USB protocol
		// inefficiencies.
		Barry::Init(env->m_DebugMode);


		/*
		 * Process Ressource options
		 */
		trace.log("Process Ressource options...");

		if (barry_calendar_initialize(env, info, error)) {
			env->m_CalendarSync.m_Sync = true;
		}
		else {
			trace.log("No sync Calendar");
			env->m_CalendarSync.m_Sync = false;
		}

		if (barry_contact_initialize(env, info, error)) {
			env->m_ContactsSync.m_Sync = true;
		}
		else {
			trace.log("No sync Contact");
			env->m_ContactsSync.m_Sync = false;
		}

		if (barry_journal_initialize(env, info, error)) {
			env->m_JournalSync.m_Sync = true;
		}
		else {
			trace.log("No sync Journal");
			env->m_JournalSync.m_Sync = false;
		}

		if (barry_todo_initialize(env, info, error)) {
			env->m_TodoSync.m_Sync = true;
		}
		else {
			trace.log("No sync Todo");
			env->m_TodoSync.m_Sync = false;
		}

		return (void *) env;
	}
	// Don't let exceptions escape to the C modules
	catch( std::bad_alloc &ba ) {
		trace.logf("Unable to allocate memory for controller: %s", ba.what());
		delete env;
		return NULL;
	}
	catch( std::exception &e ) {
		trace.logf("exception: %s", e.what());
		delete env;
		return NULL;
	}
}


static osync_bool discover(OSyncPluginInfo *info, void *userdata, OSyncError **error)
{
	Trace trace("discover");

	AutoOSyncList sinks = osync_plugin_info_get_objtype_sinks(info);
	for (OSyncList *s = sinks.Get(); s; s = s->next) {
		OSyncObjTypeSink *sink = (OSyncObjTypeSink*) s->data;

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

	trace.logf("%s(%p, %p, %p, %p)\n", __func__, sink, info, ctx, userdata);

	try {
		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// I have to test if the device is already connected.
		// Indeed, if I sync both contact and event, the connect
		// function is called two times.
		if (!env->isConnected()) {
			// Probe for available devices
			Barry::Probe probe;
			int nIndex = probe.FindActive(env->m_pin);
			if( nIndex == -1 ) {
				osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Unable to find PIN %x", env->m_pin);
				return;
			}

			trace.log("connecting...");

			env->Connect(probe.Get(nIndex));

			trace.log("connected !");
		}

		// Success!
		osync_context_report_success(ctx);

		trace.log("connect success");
	}
	// Don't let exceptions escape to the C modules
	catch( std::bad_alloc &ba ) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION,
			"Unable to allocate memory for controller: %s", ba.what());
	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION,
			"%s", e.what());
	}
}


static void contact_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
	Trace trace("contact_get_changeinfo");

	try {
		BarryEnvironment *env = (BarryEnvironment *) userdata;

		GetChanges(sink, info, ctx, env, &env->m_ContactsSync,
			"Address Book", "contact", "vcard30",
			&VCardConverter::GetRecordData,
			slow_sync);

		// Success!
		osync_context_report_success(ctx);
	}

	// don't let exceptions escape to the C modules
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}

static void event_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
	Trace trace("event_get_changeinfo");

	try {
		BarryEnvironment *env = (BarryEnvironment *) userdata;

		GetChanges(sink, info, ctx, env, &env->m_CalendarSync,
			"Calendar", "event", "vevent20",
			&VEventConverter::GetRecordData,
			slow_sync);

		// Success!
		osync_context_report_success(ctx);
	}
	// don't let exceptions escape to the C modules
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}


static void journal_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
	Trace trace("journal_get_changeinfo");

	try {
		BarryEnvironment *env = (BarryEnvironment *) userdata;

		GetChanges(sink, info, ctx, env, &env->m_JournalSync,
			"Memos", "note", "vjournal",
			&VJournalConverter::GetRecordData,
			slow_sync);

		// Success!
		osync_context_report_success(ctx);
	}
	// don't let exceptions escape to the C modules
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}


static void todo_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
	Trace trace("todo_get_changeinfo");

	try {
		BarryEnvironment *env = (BarryEnvironment *) userdata;

		GetChanges(sink, info, ctx, env, &env->m_TodoSync,
			"Tasks", "todo", "vtodo20",
			&VTodoConverter::GetRecordData,
			slow_sync);

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

		OSyncHashTable *hashtable = osync_objtype_sink_get_hashtable(sink);

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
		Barry::RecordStateTable &table = pSync->m_Table;
		Barry::Mode::Desktop &desktop = *env->GetDesktop();
		unsigned int dbId = pSync->m_dbId;


		// either generate or retrieve the record ID, based on type
		Barry::RecordStateTable::IndexType StateIndex;
		unsigned long RecordId = 0;
		if( osync_change_get_changetype(change) == OSYNC_CHANGE_TYPE_ADDED ) {
			// create new ID for this record
			RecordId = table.MakeNewRecordId();

			// tell opensync to save our ID
			char *puid = g_strdup_printf("%lu", RecordId);
			osync_change_set_uid(change, puid);
			g_free(puid);
		}
		else {
			// extract RecordId from change's UID,
			const char *uid = osync_change_get_uid(change);
			trace.logf("uid from change: %s", uid);

			// convert existing UID string to RecordId
			if( strlen(uid) == 0 ||
			    sscanf(uid, "%lu", &RecordId) != 1 ||
			    RecordId == 0)
			{
				trace.logf("Unable to extract a valid record ID from: %s", uid);
				osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "Unable to extract a valid record ID from: %s", uid);
				return;
			}

			// search for the RecordId in the state table, to find the
			// index... we only need the index if we are deleting or
			// modifying
			if( !table.GetIndex(RecordId, &StateIndex) ) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
					"unable to get state table index for RecordId: %lu",
					RecordId);
				return;
			}
		}

		// if we get here, we are about to update the device,
		// and dirty flags will change in such a way that a
		// reconnect will be required later... so flag this state
		env->RequireDirtyReconnect();


		std::string errmsg;
		bool status;

		OSyncData *odata = NULL;
		char *plain = NULL;

		switch( osync_change_get_changetype(change) )
		{
		case OSYNC_CHANGE_TYPE_DELETED:
			desktop.DeleteRecord(dbId, StateIndex);
			break;

		case OSYNC_CHANGE_TYPE_ADDED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			status = (*CommitData)(env, dbId, StateIndex, RecordId,
				plain, true, errmsg);
			if( !status ) {
				trace.logf("CommitData() for ADDED state returned false: %s", errmsg.c_str());
				osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
				return;
			}
			osync_change_set_hash(change, "0");
			break;

		case OSYNC_CHANGE_TYPE_MODIFIED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			status = (*CommitData)(env, dbId, StateIndex, RecordId,
				plain, false, errmsg);
			if( !status ) {
				trace.logf("CommitData() for MODIFIED state returned false: %s", errmsg.c_str());
				osync_context_report_error(ctx, OSYNC_ERROR_PARAMETER, "%s", errmsg.c_str());
				return;
			}
			osync_change_set_hash(change, "0");
			break;

		default:
			trace.log("Unknown change type");
			break;
		}

		// Update hashtable
		osync_hashtable_update_change(hashtable, change);

		// Answer the call
		osync_context_report_success(ctx);
		return;


	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
		return;
	}
}


static void contact_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	//
	// This function will only be called if the sync was successfull
	//

	Trace trace("contact_sync_done");

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// do cleanup for each database
		if( FinishSync(ctx, env, &env->m_ContactsSync) )
		{
			// Success
			osync_context_report_success(ctx);
		}

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}


static void event_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	//
	// This function will only be called if the sync was successfull
	//

	Trace trace("event_sync_done");

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// do cleanup for each database
		if( FinishSync(ctx, env, &env->m_CalendarSync) )
		{
			// Success
			osync_context_report_success(ctx);
		}

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}


static void journal_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	//
	// This function will only be called if the sync was successfull
	//

	Trace trace("journal_sync_done");

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// do cleanup for each database
		if( FinishSync(ctx, env, &env->m_JournalSync) )
		{
			// Success
			osync_context_report_success(ctx);
		}

	}
	catch( std::exception &e ) {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "%s", e.what());
	}
}


static void todo_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	//
	// This function will only be called if the sync was successfull
	//

	Trace trace("todo_sync_done");

	try {

		BarryEnvironment *env = (BarryEnvironment *) userdata;

		// do cleanup for each database
		if( FinishSync(ctx, env, &env->m_TodoSync) )
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
	Trace trace("contact_disconnect");

	// Done!
	osync_context_report_success(ctx);
}


static void finalize(void *data)
{
	Trace trace("finalize");

	BarryEnvironment *env = (BarryEnvironment *) data;

	// Disconnect the controller, which closes our connection
	if (env->isConnected())
		env->Disconnect();

	delete env;
}


osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	Trace trace("get_sync_info");

	// Create a new OpenSync plugin
	OSyncPlugin *plugin = osync_plugin_new(error);
	if( !plugin ) {
		trace.log(osync_error_print(error));
		return false;
	}

	// Describe our plugin
	osync_plugin_set_name(plugin, "barry-sync");
	osync_plugin_set_longname(plugin, "Barry OpenSync plugin v0.18.0 for the Blackberry handheld");
	osync_plugin_set_description(plugin, "Plugin to synchronize note, task, calendar and contact entries on USB Blackberry handhelds");

	// Set the callback functions
	osync_plugin_set_initialize_func(plugin, initialize);
	osync_plugin_set_finalize_func(plugin, finalize);
	osync_plugin_set_discover_func(plugin, discover);
	osync_plugin_set_start_type(plugin, OSYNC_START_TYPE_PROCESS);

	if( !osync_plugin_env_register_plugin(env, plugin, error) ) {
		trace.log(osync_error_print(error));
		return false;
	}

	osync_plugin_unref(plugin);

	return true;
}


int get_version(void)
{
	return 1;
}

