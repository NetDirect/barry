//
// \file	barry_sync.cc
//		Opensync module for the USB Blackberry handheld
//

/*
    Copyright (C) 2006, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <sstream>
#include <glib.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>

// All functions that are callable from outside must look like C
extern "C" {
	static void *initialize(OSyncMember *member, OSyncError **error);
	static void connect(OSyncContext *ctx);
	static void get_changeinfo(OSyncContext *ctx);
	static osync_bool commit_change(OSyncContext *ctx, OSyncChange *change);
	static void sync_done(OSyncContext *ctx);
	static void disconnect(OSyncContext *ctx);
	static void finalize(void *data);
	void get_info(OSyncEnv *env);
}

static void *initialize(OSyncMember *member, OSyncError **error)
{
	// Create the environment struct, including our Barry objects
	try {
		Barry::Init(true);

		BarryEnvironment *env = new BarryEnvironment;

		// Load config file for this plugin
		char *configdata;
		int configsize;
	//	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
	//		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
	//		delete env;
	//		return NULL;
	//	}

		// Process the configdata here and set the options on your environment
	//	free(configdata);
		env->member = member;

		// If you need a hashtable you make it here
	//	env->hashtable = osync_hashtable_new();

		return env;

	}
	catch( std::bad_alloc &ba ) {
		// Don't let C++ exceptions escape to the C code
		osync_error_update(error, "Unable to allocate memory for environment: %s", ba.what());
		return NULL;
	}
}

static void connect(OSyncContext *ctx)
{
	try {

		// Each time you get passed a context (which is used to track
		// calls to your plugin) you can get the data your returned in
		// initialize via this call:
		BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

		//
		// Now connect to your devices and report
		// 
		// an error via:
		// osync_context_report_error(ctx, ERROR_CODE, "Some message");
		// 
		// or success via:
		// osync_context_report_success(ctx);
		// 
		// You have to use one of these 2 somewhere to answer the context.
		// 
		//
		
//		// If you are using a hashtable you have to load it here
//		OSyncError *error = NULL;
//		if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
//			osync_context_report_osyncerror(ctx, &error);
//			return;
//		}
		
//		//you can also use the anchor system to detect a device reset
//		//or some parameter change here. Check the docs to see how it works
//		char *lanchor = NULL;
//		//Now you get the last stored anchor from the device
//		if (!osync_anchor_compare(env->member, "lanchor", lanchor))
//			osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);

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
	catch( Barry::BError &be ) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Barry exception: %s", be.what());
	}
	catch( std::bad_alloc &ba ) {
		// don't let exceptions escape to the C modules
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION,
			"Unable to allocate memory for controller: %s", ba.what());
	}
}

class CalendarStore
{
	// external data
	OSyncContext *ctx;
	BarryEnvironment *env;


public:
	CalendarStore(OSyncContext *c, BarryEnvironment *e)
		: ctx(c), env(e)
	{
	}

	// storage operator
	void operator()(const Barry::Calendar &rec)
	{
		// Put calendar event data into vevent20 format
		char *start = osync_time_unix2vtime(&rec.StartTime);
		char *end = osync_time_unix2vtime(&rec.EndTime);
		// FIXME - need the notification time too... where does that fit in VCALENDAR?
		char *data = g_strdup_printf(
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

		// Setup the change object for opensync
		OSyncChange *change = osync_change_new();
		osync_change_set_member(change, env->member);

		char *uid = g_strdup_printf("%lu", rec.RecordId);
		osync_change_set_uid(change, uid);
		g_free(uid);

		osync_change_set_changetype(change, CHANGE_ADDED);
		osync_change_set_objformat_string(change, "vevent20");
//		osync_change_set_hash(change, "the calculated hash of the object");

		// Now you can set the data for the object
		// Set the last argument to FALSE if the real data
		// should be queried later in a "get_data" function
		osync_change_set_data(change, data, strlen(data), TRUE);			

//		// If you use hashtables use these functions:
//		if (osync_hashtable_detect_change(env->hashtable, change)) {
//			osync_context_report_change(ctx, change);
//			osync_hashtable_update_hash(env->hashtable, change);
//		}	
		// otherwise just report the change via
		osync_context_report_change(ctx, change);
	}
};

static void get_changeinfo(OSyncContext *ctx)
{
	try {

		BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);

		// If you use opensync hashtables you can detect if you need
		// to do a slow-sync and set this on the hastable directly
		// otherwise you have to make 2 function like "get_changes" and
		// "get_all" and decide which to use using
		// osync_member_get_slow_sync
//		if( osync_member_get_slow_sync(env->member, "event") )
//			osync_hashtable_set_slow_sync(env->hashtable, "event");

		//
		// Now you can get the changes.
		// Loop over all changes you get and do the following:
		//
		CalendarStore store(ctx, env);
		env->m_pCon->LoadDatabaseByType<Barry::Calendar>(store);

		// When you are done looping and if you are using hashtables	
//		osync_hashtable_report_deleted(env->hashtable, ctx, "data");

		// Success!
		osync_context_report_success(ctx);
	}
	catch( Barry::BError &be ) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Barry exception: %s", be.what());
	}
	catch( std::bad_alloc &ba ) {
		// don't let exceptions escape to the C modules
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Misc. memory error: %s", ba.what());
	}
}

static osync_bool commit_change(OSyncContext *ctx, OSyncChange *change)
{
/*
	barry_environment *env = (barry_environment *)osync_context_get_plugin_data(ctx);
	
	//
	// Here you have to add, modify or delete a object
	// 
	//
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			//Delete the change
			//Dont forget to answer the call on error
			break;
		case CHANGE_ADDED:
			//Add the change
			//Dont forget to answer the call on error
			//If you are using hashtables you have to calculate the hash here:
			osync_change_set_hash(change, "new hash");
			break;
		case CHANGE_MODIFIED:
			//Modify the change
			//Dont forget to answer the call on error
			//If you are using hashtables you have to calculate the new hash here:
			osync_change_set_hash(change, "new hash");
			break;
		default:
			osync_debug("FILE-SYNC", 0, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	//if you use hashtable, update the hash now.
	osync_hashtable_update_hash(env->hashtable, change);
	return TRUE;
*/
}

static void sync_done(OSyncContext *ctx)
{
//	BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);
	
	//
	// This function will only be called if the sync was successfull
	//
	
	// If we have a hashtable we can now forget the already reported changes
//	osync_hashtable_forget(env->hashtable);
	
//	//If we use anchors we have to update it now.
//	char *lanchor = NULL;
//	//Now you get/calculate the current anchor of the device
//	osync_anchor_update(env->member, "lanchor", lanchor);
	
	// Success
	osync_context_report_success(ctx);
}

static void disconnect(OSyncContext *ctx)
{
	BarryEnvironment *env = (BarryEnvironment *)osync_context_get_plugin_data(ctx);
	
	// Disconnect the controller, which closes our connection
	env->Disconnect();
	
	// Close the hashtable
//	osync_hashtable_close(env->hashtable);

	// Done!
	osync_context_report_success(ctx);
}

static void finalize(void *data)
{
	BarryEnvironment *env = (BarryEnvironment *)data;

	// Free all stuff that you have allocated here.
//	osync_hashtable_free(env->hashtable);

	delete env;
}

void get_info(OSyncEnv *env)
{
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

	// Register all object types we support
//	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objtype(info, "event");

	// Register the formats for each object type
//	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_accept_objformat(info, "event", "vevent20", NULL);

	// set the commit function for this format. this function will be called for
	// each object to write once
//	osync_plugin_set_commit_objformat(info, "<object type name>", "<format name>", commit_change);

	// the other possibility is to do a batch commit by setting
	// osync_plugin_set_batch_commit_objformat(info, "<object type name>", "<format name>", batch_commit);
	// this function will be called exactly once with all objects to write gathered in an array
}

