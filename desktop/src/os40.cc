///
/// \file	os40.cc
///		Wrapper class for opensync 0.22 syncing behaviour
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

    With code from osynctool (GPL v2+):
    Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
    Copyright (C) 2006-2007  Daniel Friedrich <daniel.friedrich@opensync.org>
    Copyright (C) 2008-2009  Daniel Gollub <gollub@b1-systems.de> 

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

#include "os40.h"
#include "os22.h"
#include "osprivatebase.h"
#include "tempdir.h"
#include <barry/vsmartptr.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <errno.h>
#include <glib.h>

#include <../libopensync1/opensync/opensync.h>
#include <../libopensync1/opensync/opensync-group.h>
#include <../libopensync1/opensync/opensync-format.h>
#include <../libopensync1/opensync/opensync-plugin.h>
#include <../libopensync1/opensync/opensync-engine.h>
#include <../libopensync1/opensync/opensync-data.h>
#include <../libopensync1/opensync/opensync-capabilities.h>

using namespace std;
using namespace Barry;

namespace OpenSync {


typedef Barry::vLateSmartPtr<OSyncEngine, void(*)(OSyncEngine*)> EngineHandle;
typedef Barry::vLateSmartPtr<OSyncList, void(*)(OSyncList*)> SyncListHandle;


//
// Private class declarations
//


class TossError
{
	OSyncError *m_error;
	OpenSync40Private *m_priv;

public:
	// simple wrapper... unref's the error on destruction
	TossError(OpenSync40Private *priv)
		: m_error(0)
		, m_priv(priv)
	{
	}

	~TossError()
	{
		Clear();
	}

	/// Returns NULL if no error
	const char* GetErrorMsg();
	bool IsSet();
	void Clear();

	operator OSyncError**()
	{
		return &m_error;
	}
};

class OpenSync40Private
{
public:
	// function pointers
	const char*		(*osync_get_version)();
	const char*		(*osync_error_print)(OSyncError **error);
	osync_bool		(*osync_error_is_set)(OSyncError **error);
	void			(*osync_error_unref)(OSyncError **error);
	OSyncGroupEnv*		(*osync_group_env_new)(OSyncError **error);
	OSyncFormatEnv*		(*osync_format_env_new)(OSyncError **error);
	OSyncPluginEnv*		(*osync_plugin_env_new)(OSyncError **error);
	void			(*osync_group_env_unref)(OSyncGroupEnv *env);
	void			(*osync_format_env_unref)(OSyncFormatEnv *env);
	void			(*osync_plugin_env_unref)(OSyncPluginEnv *env);
	osync_bool		(*osync_plugin_env_load)(OSyncPluginEnv *env,
					const char *path, OSyncError **error);
	OSyncList*		(*osync_plugin_env_get_plugins)(
					OSyncPluginEnv *env);
	const char*		(*osync_plugin_get_name)(OSyncPlugin *plugin);
	void			(*osync_list_free)(OSyncList *list);
	osync_bool		(*osync_group_env_load_groups)(
					OSyncGroupEnv *env, const char *path,
					OSyncError **error);
	osync_bool		(*osync_format_env_load_plugins)(
					OSyncFormatEnv *env, const char *path,
					OSyncError **error);
	OSyncList*		(*osync_group_env_get_groups)(
					OSyncGroupEnv *env);
	const char*		(*osync_group_get_name)(OSyncGroup *group);
	OSyncGroup*		(*osync_group_env_find_group)(
					OSyncGroupEnv *env, const char *name);
	OSyncList*		(*osync_group_get_members)(OSyncGroup *group);
	const char*		(*osync_member_get_name)(OSyncMember *member);
	long long int		(*osync_member_get_id)(OSyncMember *member);
	const char*		(*osync_member_get_pluginname)(
					OSyncMember *member);
	OSyncList*		(*osync_format_env_get_objformats)(
					OSyncFormatEnv *env);
	const char*		(*osync_objformat_get_name)(
					OSyncObjFormat *format);
	const char*		(*osync_objformat_get_objtype)(
					OSyncObjFormat *format);
	OSyncGroup*		(*osync_group_new)(OSyncError **error);
	void			(*osync_group_unref)(OSyncGroup *group);
	void			(*osync_group_set_name)(OSyncGroup *group,
					const char *name);
	osync_bool		(*osync_group_env_add_group)(OSyncGroupEnv *env,
					OSyncGroup *group,
					OSyncError **error);
	osync_bool		(*osync_group_save)(OSyncGroup *group,
					OSyncError **error);
	osync_bool		(*osync_group_delete)(OSyncGroup *group,
					OSyncError **error);
	void			(*osync_group_env_remove_group)(
					OSyncGroupEnv *env, OSyncGroup *group);
	OSyncPlugin*		(*osync_plugin_env_find_plugin)(
					OSyncPluginEnv *env, const char *name);
	void			(*osync_member_unref)(OSyncMember *member);
	OSyncMember*		(*osync_member_new)(OSyncError **error);
	void			(*osync_group_add_member)(OSyncGroup *group,
					OSyncMember *member);
	void			(*osync_member_set_pluginname)(
					OSyncMember *member,
					const char *pluginname);
	void			(*osync_member_set_name)(OSyncMember *member,
					const char *name);
	osync_bool		(*osync_member_save)(OSyncMember *member,
					OSyncError **error);
	OSyncMember*		(*osync_group_find_member)(OSyncGroup *group,
					long long int id);
	osync_bool		(*osync_member_delete)(OSyncMember *member,
					OSyncError **error);
	void			(*osync_group_remove_member)(OSyncGroup *group,
					OSyncMember *member);
	OSyncPluginConfig*	(*osync_plugin_config_new)(OSyncError **error);
	osync_bool		(*osync_plugin_config_file_load)(
					OSyncPluginConfig *config,
					const char *path,
					OSyncError **error);
	void			(*osync_member_set_config)(OSyncMember *member,
					OSyncPluginConfig *config);
	OSyncPluginConfig*	(*osync_member_get_config_or_default)(
					OSyncMember *member,
					OSyncError **error);
	osync_bool		(*osync_plugin_config_file_save)(
					OSyncPluginConfig *config,
					const char *path, OSyncError **error);
	OSyncPluginConfigurationType (*osync_plugin_get_config_type)(
					OSyncPlugin *plugin);
	OSyncEngine*		(*osync_engine_new)(OSyncGroup *group,
					OSyncError **error);
	void			(*osync_engine_unref)(OSyncEngine *engine);
	osync_bool		(*osync_engine_discover_and_block)(
					OSyncEngine *engine,
					OSyncMember *member,
					OSyncError **error);
	OSyncList*		(*osync_member_get_objtypes)(
					OSyncMember *member);
	unsigned int		(*osync_list_length)(const OSyncList *list);
	void			(*osync_error_set)(OSyncError **error,
					OSyncErrorType type,
					const char *format, ...);
	osync_bool		(*osync_engine_finalize)(OSyncEngine *engine,
					OSyncError **error);
	OSyncList*		(*osync_mapping_engine_get_changes)(
					OSyncMappingEngine *engine);
	osync_bool		(*osync_mapping_engine_supports_ignore)(
					OSyncMappingEngine *engine);
	osync_bool		(*osync_mapping_engine_supports_use_latest)(
					OSyncMappingEngine *engine);
	OSyncList*		(*osync_list_nth)(OSyncList *list,
					unsigned int n);
	osync_bool		(*osync_engine_mapping_solve)(
					OSyncEngine *engine,
					OSyncMappingEngine *mapping_engine,
					OSyncChange *change,
					OSyncError **error);
	osync_bool		(*osync_engine_abort)(OSyncEngine *engine,
					OSyncError **error);
	osync_bool		(*osync_engine_mapping_duplicate)(
					OSyncEngine *engine,
					OSyncMappingEngine *mapping_engine,
					OSyncError **error);
	osync_bool		(*osync_engine_mapping_ignore_conflict)(
					OSyncEngine *engine,
					OSyncMappingEngine *mapping_engine,
					OSyncError **error);
	osync_bool		(*osync_engine_mapping_use_latest)(
					OSyncEngine *engine,
					OSyncMappingEngine *mapping_engine,
					OSyncError **error);
	OSyncChangeType		(*osync_change_get_changetype)(
					OSyncChange *change);
	OSyncMember*		(*osync_mapping_engine_change_find_member)(
					OSyncMappingEngine *engine,
					OSyncChange *change);
	OSyncData*		(*osync_change_get_data)(OSyncChange *change);
	char*			(*osync_data_get_printable)(OSyncData *data,
					OSyncError **error);
	void			(*osync_free)(void *ptr);
	const char*		(*osync_change_get_uid)(OSyncChange *change);
	osync_bool		(*osync_engine_continue)(OSyncEngine *engine,
					OSyncError **error);
	OSyncList*		(*osync_engine_get_objengines)(
					OSyncEngine *engine);
	OSyncList*		(*osync_obj_engine_get_members)(
					OSyncObjEngine* engine);
	const char*		(*osync_obj_engine_get_objtype)(
					OSyncObjEngine *engine);
	const OSyncList*	(*osync_obj_engine_get_mapping_entry_engines_of_member)(
					OSyncObjEngine *engine,
					OSyncMember *member);
	osync_bool		(*osync_entry_engine_is_dirty)(
					OSyncMappingEntryEngine *engine);
	OSyncChangeType		(*osync_entry_engine_get_changetype)(
					OSyncMappingEntryEngine *engine);
	OSyncChange*		(*osync_engine_change_update_get_change)(
					OSyncEngineChangeUpdate *update);
	OSyncMember*		(*osync_engine_change_update_get_member)(
					OSyncEngineChangeUpdate *update);
	OSyncError*		(*osync_engine_change_update_get_error)(
					OSyncEngineChangeUpdate *update);
	OSyncEngineChangeEvent	(*osync_engine_change_update_get_event)(
					OSyncEngineChangeUpdate *update);
	OSyncObjFormat*		(*osync_change_get_objformat)(
					OSyncChange *change);
	OSyncError*		(*osync_engine_mapping_update_get_error)(
					OSyncEngineMappingUpdate *update);
	OSyncError*		(*osync_engine_update_get_error)(
					OSyncEngineUpdate *update);
	OSyncEngineEvent	(*osync_engine_update_get_event)(
					OSyncEngineUpdate *update);
	const char*		(*osync_engine_member_update_get_objtype)(
					OSyncEngineMemberUpdate *update);
	OSyncMember*		(*osync_engine_member_update_get_member)(
					OSyncEngineMemberUpdate *update);
	OSyncError*		(*osync_engine_member_update_get_error)(
					OSyncEngineMemberUpdate *update);
	OSyncEngineMemberEvent	(*osync_engine_member_update_get_event)(
					OSyncEngineMemberUpdate *update);
	void			(*osync_engine_set_conflict_callback)(
					OSyncEngine *engine,
					osync_conflict_cb callback,
					void *user_data);
	void			(*osync_engine_set_changestatus_callback)(
					OSyncEngine *engine,
					osync_status_change_cb callback,
					void *user_data);
	void			(*osync_engine_set_mappingstatus_callback)(
					OSyncEngine *engine,
					osync_status_mapping_cb callback,
					void *user_data);
	void			(*osync_engine_set_enginestatus_callback)(
					OSyncEngine *engine,
					osync_status_engine_cb callback,
					void *user_data);
	void			(*osync_engine_set_memberstatus_callback)(
					OSyncEngine *engine,
					osync_status_member_cb callback,
					void *user_data);
	void			(*osync_engine_set_multiply_callback)(
					OSyncEngine *engine,
					osync_multiply_cb callback,
					void *user_data);
	osync_bool		(*osync_engine_initialize)(OSyncEngine *engine,
					OSyncError **error);
	osync_bool		(*osync_engine_synchronize_and_block)(
					OSyncEngine *engine,OSyncError **error);
	OSyncEngineMappingEvent	(*osync_engine_mapping_update_get_event)(
					OSyncEngineMappingUpdate *update);

	// data pointers
	vLateSmartPtr<OSyncGroupEnv, void(*)(OSyncGroupEnv*)> group_env;
	vLateSmartPtr<OSyncFormatEnv, void(*)(OSyncFormatEnv*)> format_env;
	vLateSmartPtr<OSyncPluginEnv, void(*)(OSyncPluginEnv*)> plugin_env;

	TossError error;

	OpenSync40Private()
		: error(this)
	{
	}
};

class SyncConflict40Private : public SyncConflictPrivateBase
{
	OpenSync40Private *m_priv;
	OSyncEngine *m_engine;
	OSyncMappingEngine *m_mapping;
	OSyncList *m_changes;

public:
	SyncConflict40Private(OpenSync40Private *priv,
		OSyncEngine *engine, OSyncMappingEngine *mapping);
	~SyncConflict40Private();

	virtual bool IsIgnoreSupported() const;
	virtual bool IsKeepNewerSupported() const;

	virtual void Select(int change_index);
	virtual void Abort();
	virtual void Duplicate();
	virtual void Ignore();
	virtual void KeepNewer();

	void AppendChanges(std::vector<SyncChange> &list);
};

class SyncSummary40Private : public SyncSummaryPrivateBase
{
	OpenSync40Private *m_priv;
	OSyncEngine *m_engine;

public:
	SyncSummary40Private(OpenSync40Private *priv, OSyncEngine *engine);
	~SyncSummary40Private();

	virtual void Abort();
	virtual void Continue();

	// returns true if any member is dirty
	bool AppendMembers(std::vector<SyncMemberSummary> &list);
};

struct CallbackBundle
{
	OpenSync40Private *m_priv;
	SyncStatus *m_status;

	CallbackBundle(OpenSync40Private *priv, SyncStatus &status)
		: m_priv(priv)
		, m_status(&status)
	{
	}
};

void conflict_handler(OSyncEngine *, OSyncMappingEngine *, void *);
void entry_status(OSyncEngineChangeUpdate *, void *);
void mapping_status(OSyncEngineMappingUpdate *, void *);
void engine_status(OSyncEngineUpdate *, void *);
void member_status(OSyncEngineMemberUpdate *, void *);
void multiply_summary(OSyncEngine *, void *);


/////////////////////////////////////////////////////////////////////////////
// Static helper functions

static const char *OSyncChangeType2String(OSyncChangeType type)
{
	switch (type) {
		case OSYNC_CHANGE_TYPE_ADDED: return "ADDED";
		case OSYNC_CHANGE_TYPE_UNMODIFIED: return "UNMODIFIED";
		case OSYNC_CHANGE_TYPE_DELETED: return "DELETED";
		case OSYNC_CHANGE_TYPE_MODIFIED: return "MODIFIED";
		default:
		case OSYNC_CHANGE_TYPE_UNKNOWN: return "?";
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// SyncConflict40Private member functions

SyncConflict40Private::SyncConflict40Private(OpenSync40Private *priv,
			OSyncEngine *engine, OSyncMappingEngine *mapping)
	: m_priv(priv)
	, m_engine(engine)
	, m_mapping(mapping)
	, m_changes(0)
{
	m_changes = m_priv->osync_mapping_engine_get_changes(m_mapping);
}

SyncConflict40Private::~SyncConflict40Private()
{
	m_priv->osync_list_free(m_changes);
}

bool SyncConflict40Private::IsIgnoreSupported() const
{
	return m_priv->osync_mapping_engine_supports_ignore(m_mapping);
}

bool SyncConflict40Private::IsKeepNewerSupported() const
{
	return m_priv->osync_mapping_engine_supports_use_latest(m_mapping);
}

void SyncConflict40Private::Select(int change_index)
{
	OSyncList *c = m_priv->osync_list_nth(m_changes, change_index);
	if( !c )
		throw std::logic_error("Bad change_index");

	OSyncChange *change = (OSyncChange *) c->data;

	if( !m_priv->osync_engine_mapping_solve(m_engine, m_mapping,
		change, m_priv->error) )
	{
		throw std::runtime_error(m_priv->error.GetErrorMsg());
	}
}

void SyncConflict40Private::Abort()
{
	if( !m_priv->osync_engine_abort(m_engine, m_priv->error) ) {
		ostringstream oss;
		oss << "Problems while aborting: "
			<< m_priv->error.GetErrorMsg();
		throw std::runtime_error(oss.str());
	}
}

void SyncConflict40Private::Duplicate()
{
	if( !m_priv->osync_engine_mapping_duplicate(m_engine, m_mapping, m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}

void SyncConflict40Private::Ignore()
{
	if( !IsIgnoreSupported() )
		throw std::logic_error("Ignore not supported, yet Ignore() called.");

	if( !m_priv->osync_engine_mapping_ignore_conflict(m_engine, m_mapping,
		m_priv->error) )
	{
		throw std::runtime_error(m_priv->error.GetErrorMsg());
	}
}

void SyncConflict40Private::KeepNewer()
{
	if( !IsKeepNewerSupported() )
		throw std::logic_error("Keep Newer not supported, yet KeepNewer() called.");

	if( !m_priv->osync_engine_mapping_use_latest(m_engine, m_mapping, m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}

void SyncConflict40Private::AppendChanges(std::vector<SyncChange> &list)
{
	int i = 0;
	for( OSyncList *c = m_changes; c; c = c->next, i++ ) {
		OSyncChange *change = (OSyncChange *) c->data;

		if( m_priv->osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_UNKNOWN ) {
			OSyncMember *member = m_priv->osync_mapping_engine_change_find_member(m_mapping, change);

			OSyncData *data = m_priv->osync_change_get_data(change);

			SyncChange entry;

			char *printable = m_priv->osync_data_get_printable(data, m_priv->error);
			if( printable )
				entry.printable_data = printable;
			m_priv->osync_free(printable);

			if( m_priv->error.IsSet() )
				throw std::runtime_error(m_priv->error.GetErrorMsg());

			entry.id = i;
			entry.member_id = m_priv->osync_member_get_id(member);
			if( entry.member_id != m_priv->osync_member_get_id(member) )
				throw std::logic_error("long can't hold long long");

			entry.plugin_name = m_priv->osync_member_get_pluginname(member);
			entry.uid = m_priv->osync_change_get_uid(change);

			// add to list
			list.push_back(entry);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// SyncSummary40Private member functions

SyncSummary40Private::SyncSummary40Private(OpenSync40Private *priv,
						OSyncEngine *engine)
	: m_priv(priv)
	, m_engine(engine)
{
}

SyncSummary40Private::~SyncSummary40Private()
{
}

void SyncSummary40Private::Abort()
{
	if( !m_priv->osync_engine_abort(m_engine, m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}

void SyncSummary40Private::Continue()
{
	if( !m_priv->osync_engine_continue(m_engine, m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}

bool SyncSummary40Private::AppendMembers(std::vector<SyncMemberSummary> &list)
{
	SyncListHandle objengines(m_priv->osync_list_free);
	objengines = m_priv->osync_engine_get_objengines(m_engine);

	int i = 0;
	bool dirty = false;

	for( OSyncList *o = objengines.get(); o; o = o->next ) {
		OSyncObjEngine *objengine = (OSyncObjEngine *) o->data;





		SyncListHandle members(m_priv->osync_list_free);
		members = m_priv->osync_obj_engine_get_members(objengine);
		for( OSyncList *m = members.get(); m; m = m->next ) {
			OSyncMember *member = (OSyncMember *) m->data;

			// Fill in common summary data
			SyncMemberSummary entry;
			entry.id = i;
			entry.objtype_name = m_priv->osync_obj_engine_get_objtype(objengine);
			entry.member_id = m_priv->osync_member_get_id(member);
			if( entry.member_id != m_priv->osync_member_get_id(member) )
				throw std::logic_error("long can't hold long long");

			entry.plugin_name = m_priv->osync_member_get_pluginname(member);

			const OSyncList *mapping_entry_engines = m_priv->osync_obj_engine_get_mapping_entry_engines_of_member(objengine, member);

			// Calculate summary counts
			for( const OSyncList *e = mapping_entry_engines; e; e = e->next ) {
				OSyncMappingEntryEngine *entry_engine = (OSyncMappingEntryEngine*) e->data;

				if( !m_priv->osync_entry_engine_is_dirty(entry_engine) )
					continue;

				dirty = true;

				OSyncChangeType type = m_priv->osync_entry_engine_get_changetype(entry_engine);
				switch (type)
				{
				case OSYNC_CHANGE_TYPE_ADDED:
					entry.added++;
					break;
				case OSYNC_CHANGE_TYPE_MODIFIED:
					entry.modified++;
					break;
				case OSYNC_CHANGE_TYPE_DELETED:
					entry.deleted++;
					break;
				default:
					break;
				}
			}

			// Add entry to list
			list.push_back(entry);

		}
	}

	return dirty;
}


/////////////////////////////////////////////////////////////////////////////
// Callback functions

void conflict_handler(OSyncEngine *engine, OSyncMappingEngine *mapping,
			void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		// build the SyncConflict object
		SyncConflict40Private scp(cb->m_priv, engine, mapping);
		SyncConflict conflict(scp);

		// append all conflicting changes as vector objects in the same
		// order as the opensync library mapping
		scp.AppendChanges(conflict);

		// call the status handler
		cb->m_status->HandleConflict(conflict);
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string("Conflict not resolved. ") + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			"Unknown exception caught in conflict_handler()");
	}
}

void entry_status(OSyncEngineChangeUpdate *status, void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		ostringstream oss;

		OSyncChange *change = cb->m_priv->osync_engine_change_update_get_change(status);
		OSyncMember *member = cb->m_priv->osync_engine_change_update_get_member(status);
		OSyncError *error = cb->m_priv->osync_engine_change_update_get_error(status);

		const char *action = NULL;
		const char *direction = NULL;
		const char *msg = NULL;
		bool error_event = false;

		switch( cb->m_priv->osync_engine_change_update_get_event(status) )
		{
		case OSYNC_ENGINE_CHANGE_EVENT_READ:
			action = "Received an entry";
			direction = "from";
			msg = OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(change));
			break;

		case OSYNC_ENGINE_CHANGE_EVENT_WRITTEN:
			action = "Sent an entry";
			direction = "to";
			msg = OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(change));
			break;

		case OSYNC_ENGINE_CHANGE_EVENT_ERROR:
			error_event = true;
			action = "Error for entry";
			direction = "and";
			msg = cb->m_priv->osync_error_print(&(error));
			break;
		}

		if( action ) {
			oss << action << " "
			    << cb->m_priv->osync_change_get_uid(change)
			    << "("
			    << cb->m_priv->osync_objformat_get_name( cb->m_priv->osync_change_get_objformat(change))
			    << ") " << direction << " member "
			    << cb->m_priv->osync_member_get_id(member)
			    << " ("
			    << cb->m_priv->osync_member_get_pluginname(member)
			    << "): "
			    << msg;

			// call the status handler
			cb->m_status->EntryStatus(oss.str(), error_event);
		}
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string("entry_status error:") + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			"Unknown exception caught in entry_status()");
	}
}

void mapping_status(OSyncEngineMappingUpdate *status, void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		OSyncError *error = cb->m_priv->osync_engine_mapping_update_get_error(status);

		ostringstream oss;
		bool error_event = false;

		switch( cb->m_priv->osync_engine_mapping_update_get_event(status) )
		{
		case OSYNC_ENGINE_MAPPING_EVENT_SOLVED:
			oss << "Mapping solved";
			break;

		case OSYNC_ENGINE_MAPPING_EVENT_ERROR:
			error_event = true;
			oss << "Mapping error: "
			    << cb->m_priv->osync_error_print(&(error));
			break;
		}

		// call the status handler
		if( oss.str().size() )
			cb->m_status->MappingStatus(oss.str(), error_event);
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string("mapping_status error: ") + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			"Unknown exception caught in mapping_status()");
	}
}

void engine_status(OSyncEngineUpdate *status, void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		OSyncError *error = cb->m_priv->osync_engine_update_get_error(status);

		ostringstream oss;
		bool error_event = false;

		switch( cb->m_priv->osync_engine_update_get_event(status) )
		{
		case OSYNC_ENGINE_EVENT_CONNECTED:
			oss << "All clients connected or error";
			break;
		case OSYNC_ENGINE_EVENT_CONNECT_DONE:
			/* Not of interest for regular user. */
			break;
		case OSYNC_ENGINE_EVENT_READ:
			oss << "All clients sent changes or error";
			break;
		case OSYNC_ENGINE_EVENT_MAPPED:
			oss << "All changes got mapped";
			break;
		case OSYNC_ENGINE_EVENT_MULTIPLIED:
			oss << "All changes got multiplied";
			break;
		case OSYNC_ENGINE_EVENT_PREPARED_WRITE:
			oss << "All changes got prepared for write";
			break;
		case OSYNC_ENGINE_EVENT_PREPARED_MAP:
			/* Not of interest for regular user. */
			break;
		case OSYNC_ENGINE_EVENT_WRITTEN:
			oss << "All clients have written";
			break;
		case OSYNC_ENGINE_EVENT_DISCONNECTED:
			oss << "All clients have disconnected";
			break;
		case OSYNC_ENGINE_EVENT_ERROR:
			error_event = true;
			oss << "The sync failed: " << cb->m_priv->osync_error_print(&(error));
			break;
		case OSYNC_ENGINE_EVENT_SUCCESSFUL:
			oss << "The sync was successful";
			break;
		case OSYNC_ENGINE_EVENT_PREV_UNCLEAN:
			oss << "The previous synchronization was unclean. Slow-syncing";
			break;
		case OSYNC_ENGINE_EVENT_END_CONFLICTS:
			oss << "All conflicts have been reported";
			break;
		case OSYNC_ENGINE_EVENT_SYNC_DONE:
			oss << "All clients reported sync done";
			break;
		}

		// call the status handler
		if( oss.str().size() )
			cb->m_status->EngineStatus(oss.str(), error_event);
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string("engine_status error: ") + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			"Unknown exception caught in engine_status()");
	}
}

void member_status(OSyncEngineMemberUpdate *status, void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		ostringstream oss;
		bool error_event = false;
		bool valid = true;

		const char *objtype = cb->m_priv->osync_engine_member_update_get_objtype(status);
		if( objtype == NULL )
			oss << "Main sink";
		else
			oss << objtype << " sink";


		OSyncMember *member = cb->m_priv->osync_engine_member_update_get_member(status);

		oss << " of member "
		    << cb->m_priv->osync_member_get_id(member)
		    << " ("
		    << cb->m_priv->osync_member_get_pluginname(member)
		    << ")";

		OSyncError *error = cb->m_priv->osync_engine_member_update_get_error(status);

		switch( cb->m_priv->osync_engine_member_update_get_event(status) )
		{
		case OSYNC_ENGINE_MEMBER_EVENT_CONNECTED:
			oss << " just connected";
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_CONNECT_DONE:
			// Special event - but not interesting for
			// the normal user.
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_DISCONNECTED:
			oss << " just disconnected";
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_READ:
			oss << " just sent all changes";
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_WRITTEN:
			oss << " committed all changes";
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_SYNC_DONE:
			oss << " reported sync done";
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_DISCOVERED:
			oss << " discovered its objtypes";
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_ERROR:
			oss << " had an error: "
			    << cb->m_priv->osync_error_print(&error);
			break;
		default:
			valid = false;
			break;
		}

		// call the status handler
		if( oss.str().size() && valid ) {
			cb->m_status->MemberStatus(
				cb->m_priv->osync_member_get_id(member),
				cb->m_priv->osync_member_get_pluginname(member),
				oss.str(), error_event);
		}
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string("member_status error: ") + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			"Unknown exception caught in member_status()");
	}
}

void multiply_summary(OSyncEngine *engine, void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		// build the SyncSummary object
		SyncSummary40Private ssp(cb->m_priv, engine);
		SyncSummary summary(ssp);

		// append a summary for each objtype member
		if( ssp.AppendMembers(summary) ) {
			// call the status handler only if dirty
			cb->m_status->CheckSummary(summary);
		}
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string("Error handling summary. ") + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			"Unknown exception caught in multiply_summary()");
	}
}


/////////////////////////////////////////////////////////////////////////////
// OpenSync40 (API override class) - public members

OpenSync40::OpenSync40()
{
	// due to bugs in the way opensync 0.22 loads its modules,
	// (i.e. it doesn't load the plugins using RTLD_LOCAL, and
	// so libopensync.so.0 is loaded and pollutes the symbol table,
	// causing symbol clashes) we need to make sure that
	// OpenSync40 is only loaded first... if OpenSync22 was
	// loaded first, we will fail, so error out
	if( OpenSync22::SymbolsLoaded() )
		throw std::logic_error("Always load OpenSync40 before OpenSync22, to avoid symbol table conflicts.");

	if( !Open("libopensync.so.1") )
		throw DlError("Can't dlopen libopensync.so.1");

	// store locally in case of constructor exception in LoadSym
	std::auto_ptr<OpenSync40Private> p(new OpenSync40Private);

	// load all required symbols...
	// we don't need to use try/catch here, since the base
	// class destructor will clean up for us if LoadSym() throws
	LoadSym(p->osync_get_version, "osync_get_version");
	LoadSym(p->osync_error_print, "osync_error_print");
	LoadSym(p->osync_error_is_set, "osync_error_is_set");
	LoadSym(p->osync_error_unref, "osync_error_unref");
	LoadSym(p->osync_group_env_new, "osync_group_env_new");
	LoadSym(p->osync_format_env_new, "osync_format_env_new");
	LoadSym(p->osync_plugin_env_new, "osync_plugin_env_new");
	LoadSym(p->osync_group_env_unref, "osync_group_env_unref");
	LoadSym(p->osync_format_env_unref, "osync_format_env_unref");
	LoadSym(p->osync_plugin_env_unref, "osync_plugin_env_unref");
	LoadSym(p->osync_plugin_env_load, "osync_plugin_env_load");
	LoadSym(p->osync_plugin_env_get_plugins,"osync_plugin_env_get_plugins");
	LoadSym(p->osync_plugin_get_name, "osync_plugin_get_name");
	LoadSym(p->osync_list_free, "osync_list_free");
	LoadSym(p->osync_group_env_load_groups, "osync_group_env_load_groups");
	LoadSym(p->osync_format_env_load_plugins,
				"osync_format_env_load_plugins");
	LoadSym(p->osync_group_env_get_groups, "osync_group_env_get_groups");
	LoadSym(p->osync_group_get_name, "osync_group_get_name");
	LoadSym(p->osync_group_env_find_group, "osync_group_env_find_group");
	LoadSym(p->osync_group_get_members, "osync_group_get_members");
	LoadSym(p->osync_member_get_name, "osync_member_get_name");
	LoadSym(p->osync_member_get_id, "osync_member_get_id");
	LoadSym(p->osync_member_get_pluginname, "osync_member_get_pluginname");
	LoadSym(p->osync_format_env_get_objformats,
				"osync_format_env_get_objformats");
	LoadSym(p->osync_objformat_get_name, "osync_objformat_get_name");
	LoadSym(p->osync_objformat_get_objtype, "osync_objformat_get_objtype");
	LoadSym(p->osync_group_new, "osync_group_new");
	LoadSym(p->osync_group_unref, "osync_group_unref");
	LoadSym(p->osync_group_set_name, "osync_group_set_name");
	LoadSym(p->osync_group_env_add_group, "osync_group_env_add_group");
	LoadSym(p->osync_group_save, "osync_group_save");
	LoadSym(p->osync_group_delete, "osync_group_delete");
	LoadSym(p->osync_group_env_remove_group,"osync_group_env_remove_group");
	LoadSym(p->osync_plugin_env_find_plugin,"osync_plugin_env_find_plugin");
	LoadSym(p->osync_member_unref, "osync_member_unref");
	LoadSym(p->osync_member_new, "osync_member_new");
	LoadSym(p->osync_group_add_member, "osync_group_add_member");
	LoadSym(p->osync_member_set_pluginname, "osync_member_set_pluginname");
	LoadSym(p->osync_member_set_name, "osync_member_set_name");
	LoadSym(p->osync_member_save, "osync_member_save");
	LoadSym(p->osync_group_find_member, "osync_group_find_member");
	LoadSym(p->osync_member_delete, "osync_member_delete");
	LoadSym(p->osync_group_remove_member, "osync_group_remove_member");
	LoadSym(p->osync_plugin_config_new, "osync_plugin_config_new");
	LoadSym(p->osync_plugin_config_file_load,
				"osync_plugin_config_file_load");
	LoadSym(p->osync_member_set_config, "osync_member_set_config");
	LoadSym(p->osync_member_get_config_or_default,
				"osync_member_get_config_or_default");
	LoadSym(p->osync_plugin_config_file_save,
				"osync_plugin_config_file_save");
	LoadSym(p->osync_plugin_get_config_type,"osync_plugin_get_config_type");
	LoadSym(p->osync_engine_new, "osync_engine_new");
	LoadSym(p->osync_engine_unref, "osync_engine_unref");
	LoadSym(p->osync_engine_discover_and_block,
				"osync_engine_discover_and_block");
	LoadSym(p->osync_member_get_objtypes, "osync_member_get_objtypes");
	LoadSym(p->osync_list_length, "osync_list_length");
	LoadSym(p->osync_error_set, "osync_error_set");
	LoadSym(p->osync_engine_finalize, "osync_engine_finalize");
	LoadSym(p->osync_mapping_engine_get_changes,
				"osync_mapping_engine_get_changes");
	LoadSym(p->osync_mapping_engine_supports_ignore,
				"osync_mapping_engine_supports_ignore");
	LoadSym(p->osync_mapping_engine_supports_use_latest,
				"osync_mapping_engine_supports_use_latest");
	LoadSym(p->osync_list_nth, "osync_list_nth");
	LoadSym(p->osync_engine_mapping_solve, "osync_engine_mapping_solve");
	LoadSym(p->osync_engine_abort, "osync_engine_abort");
	LoadSym(p->osync_engine_mapping_duplicate,
				"osync_engine_mapping_duplicate");
	LoadSym(p->osync_engine_mapping_ignore_conflict,
				"osync_engine_mapping_ignore_conflict");
	LoadSym(p->osync_engine_mapping_use_latest,
				"osync_engine_mapping_use_latest");
	LoadSym(p->osync_change_get_changetype, "osync_change_get_changetype");
	LoadSym(p->osync_mapping_engine_change_find_member,
				"osync_mapping_engine_change_find_member");
	LoadSym(p->osync_change_get_data, "osync_change_get_data");
	LoadSym(p->osync_data_get_printable, "osync_data_get_printable");
	LoadSym(p->osync_free, "osync_free");
	LoadSym(p->osync_change_get_uid, "osync_change_get_uid");
	LoadSym(p->osync_engine_continue, "osync_engine_continue");
	LoadSym(p->osync_engine_get_objengines, "osync_engine_get_objengines");
	LoadSym(p->osync_obj_engine_get_members,
				"osync_obj_engine_get_members");
	LoadSym(p->osync_obj_engine_get_objtype,
				"osync_obj_engine_get_objtype");
	LoadSym(p->osync_obj_engine_get_mapping_entry_engines_of_member,
			"osync_obj_engine_get_mapping_entry_engines_of_member");
	LoadSym(p->osync_entry_engine_is_dirty,"osync_entry_engine_is_dirty");
	LoadSym(p->osync_entry_engine_get_changetype,
				"osync_entry_engine_get_changetype");
	LoadSym(p->osync_engine_change_update_get_change,
				"osync_engine_change_update_get_change");
	LoadSym(p->osync_engine_change_update_get_member,
				"osync_engine_change_update_get_member");
	LoadSym(p->osync_engine_change_update_get_error,
				"osync_engine_change_update_get_error");
	LoadSym(p->osync_engine_change_update_get_event,
				"osync_engine_change_update_get_event");
	LoadSym(p->osync_change_get_objformat, "osync_change_get_objformat");
	LoadSym(p->osync_engine_mapping_update_get_error,
				"osync_engine_mapping_update_get_error");
	LoadSym(p->osync_engine_update_get_error,
				"osync_engine_update_get_error");
	LoadSym(p->osync_engine_update_get_event,
				"osync_engine_update_get_event");
	LoadSym(p->osync_engine_member_update_get_objtype,
				"osync_engine_member_update_get_objtype");
	LoadSym(p->osync_engine_member_update_get_member,
				"osync_engine_member_update_get_member");
	LoadSym(p->osync_engine_member_update_get_error,
				"osync_engine_member_update_get_error");
	LoadSym(p->osync_engine_member_update_get_event,
				"osync_engine_member_update_get_event");
	LoadSym(p->osync_engine_set_conflict_callback,
				"osync_engine_set_conflict_callback");
	LoadSym(p->osync_engine_set_changestatus_callback,
				"osync_engine_set_changestatus_callback");
	LoadSym(p->osync_engine_set_mappingstatus_callback,
				"osync_engine_set_mappingstatus_callback");
	LoadSym(p->osync_engine_set_enginestatus_callback,
				"osync_engine_set_enginestatus_callback");
	LoadSym(p->osync_engine_set_memberstatus_callback,
				"osync_engine_set_memberstatus_callback");
	LoadSym(p->osync_engine_set_multiply_callback,
				"osync_engine_set_multiply_callback");
	LoadSym(p->osync_engine_initialize, "osync_engine_initialize");
	LoadSym(p->osync_engine_synchronize_and_block,
				"osync_engine_synchronize_and_block");
	LoadSym(p->osync_engine_mapping_update_get_event,
				"osync_engine_mapping_update_get_event");

	// fixup free pointers
	p->group_env.SetFreeFunc(p->osync_group_env_unref);
	p->format_env.SetFreeFunc(p->osync_format_env_unref);
	p->plugin_env.SetFreeFunc(p->osync_plugin_env_unref);

	// setup opensync support environment
	SetupEnvironment(p.get());

	// this pointer is ours now
	m_priv = p.release();
}

OpenSync40::~OpenSync40()
{
	delete m_priv;
	m_priv = 0;
}

void OpenSync40::SetupEnvironment(OpenSync40Private *p)
{
	// allocate group, format, and env
	p->group_env = p->osync_group_env_new(p->error);
	if( !p->group_env.get() )
		throw std::runtime_error(p->error.GetErrorMsg());

	p->format_env = p->osync_format_env_new(p->error);
	if( !p->format_env.get() )
		throw std::runtime_error(p->error.GetErrorMsg());

	p->plugin_env = p->osync_plugin_env_new(p->error);
	if( !p->plugin_env.get() )
		throw std::runtime_error(p->error.GetErrorMsg());

	// load group, format, and env
	if( !p->osync_group_env_load_groups(p->group_env.get(), NULL, p->error) ||
	    !p->osync_format_env_load_plugins(p->format_env.get(), NULL, p->error) ||
	    !p->osync_plugin_env_load(p->plugin_env.get(), NULL, p->error) )
		throw std::runtime_error(p->error.GetErrorMsg());
}

const char* OpenSync40::GetVersion() const
{
	return m_priv->osync_get_version();
}

void OpenSync40::GetPluginNames(string_list_type &plugins)
{
	// start fresh
	plugins.clear();

	OSyncPlugin *plugin;
	OSyncList *plugin_list, *p;

	plugin_list = m_priv->osync_plugin_env_get_plugins(m_priv->plugin_env.get());
	for( p = plugin_list; p; p = p->next ) {
		plugin = (OSyncPlugin *) p->data;
		plugins.push_back(m_priv->osync_plugin_get_name(plugin));
	}

	m_priv->osync_list_free(plugin_list);
}

void OpenSync40::GetFormats(format_list_type &formats)
{
	// start fresh
	formats.clear();

	OSyncList *o, *list = m_priv->osync_format_env_get_objformats(m_priv->format_env.get());

	for( o = list; o; o = o->next ) {
		OSyncObjFormat *format = (OSyncObjFormat *) o->data;

		Format new_format;
		new_format.name = m_priv->osync_objformat_get_name(format);
		new_format.object_type = m_priv->osync_objformat_get_objtype(format);

		formats.push_back(new_format);
	}

	m_priv->osync_list_free(list);
}

void OpenSync40::GetGroupNames(string_list_type &groups)
{
	// start fresh
	groups.clear();

	OSyncGroup *group;
	OSyncList *g, *group_list = m_priv->osync_group_env_get_groups(m_priv->group_env.get());

	for( g = group_list; g; g = g->next ) {
		group = (OSyncGroup *) g->data;
		groups.push_back(m_priv->osync_group_get_name(group));
	}

	m_priv->osync_list_free(group_list);
}

void OpenSync40::GetMembers(const std::string &group_name,
			member_list_type &members)
{
	// start fresh
	members.clear();

	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group ) {
		ostringstream oss;
		oss << "GetMembers: Unable to find group with name: " << group_name;
		throw std::runtime_error(oss.str());
	}

	OSyncList *member_list = m_priv->osync_group_get_members(group);
	for( OSyncList *m = member_list; m; m = m->next ) {
		Member new_member;
		OSyncMember *member = (OSyncMember *) m->data;
		const char *membername = m_priv->osync_member_get_name(member);
		if (membername) {
			new_member.friendly_name = membername;
		}

		new_member.id = m_priv->osync_member_get_id(member);
		new_member.plugin_name = m_priv->osync_member_get_pluginname(member);

		// we are switching opensync's long long int ID to
		// our long... to double check they are equal after
		// the conversion
		if( new_member.id != m_priv->osync_member_get_id(member) ) {
			m_priv->osync_list_free(member_list);
			throw std::logic_error("OpenSync's member ID is too large to fit in OpenSyncAPI (40)");
		}

		// add to member list
		members.push_back(new_member);
	}

	// cleanup
	m_priv->osync_list_free(member_list);
}

void OpenSync40::AddGroup(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_group_new(m_priv->error);
	if( !group )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	m_priv->osync_group_set_name(group, group_name.c_str());
	if( !m_priv->osync_group_env_add_group(m_priv->group_env.get(), group, m_priv->error) ) {
		m_priv->osync_group_unref(group);
		throw std::runtime_error(m_priv->error.GetErrorMsg());
	}

	if( !m_priv->osync_group_save(group, m_priv->error) ) {
		m_priv->osync_group_unref(group);
		throw std::runtime_error(m_priv->error.GetErrorMsg());
	}

	m_priv->osync_group_unref(group);
}

void OpenSync40::DeleteGroup(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	if( !m_priv->osync_group_delete(group, m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	m_priv->osync_group_env_remove_group(m_priv->group_env.get(), group);
}

void OpenSync40::AddMember(const std::string &group_name,
			const std::string &plugin_name,
			const std::string &member_name)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), plugin_name.c_str());
	if( !plugin )
		throw std::runtime_error("Plugin not found: " + plugin_name);

	vLateSmartPtr<OSyncMember, void(*)(OSyncMember*)> mptr(m_priv->osync_member_unref);
	mptr = m_priv->osync_member_new(m_priv->error);
	if( !mptr.get() )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	m_priv->osync_group_add_member(group, mptr.get());
	m_priv->osync_member_set_pluginname(mptr.get(), plugin_name.c_str());

	if( member_name.size() )
		m_priv->osync_member_set_name(mptr.get(), member_name.c_str());

	if( !m_priv->osync_member_save(mptr.get(), m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}

void OpenSync40::DeleteMember(const std::string &group_name, long member_id)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	if( !m_priv->osync_member_delete(member, m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	m_priv->osync_group_remove_member(group, member);
}

void OpenSync40::DeleteMember(const std::string &group_name,
				const std::string &plugin_name)
{
	member_list_type mlist;
	GetMembers(group_name, mlist);
	Member *member = mlist.Find(plugin_name.c_str());
	if( !member )
		throw std::runtime_error("Member not found: " + plugin_name);

	DeleteMember(group_name, member->id);
}

bool OpenSync40::IsConfigurable(const std::string &group_name,
				long member_id)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


	OSyncPluginConfigurationType type = m_priv->osync_plugin_get_config_type(plugin);
	return type != OSYNC_PLUGIN_NO_CONFIGURATION;
}

std::string OpenSync40::GetConfiguration(const std::string &group_name,
					long member_id)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "Member " << member_id << " of group '" << group_name << "' does not accept configuration.";
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


	OSyncPluginConfig *config = m_priv->osync_member_get_config_or_default(member, m_priv->error);
	if( !config )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	// To emulate 0.22 behaviour, we need to use 0.4x save-to-file
	// functions, and then load that from the file again, and
	// return that string as the configuratin.

	TempDir tempdir("opensyncapi");

	string filename = tempdir.GetNewFilename();

	if( !m_priv->osync_plugin_config_file_save(config, filename.c_str(), m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	ifstream in(filename.c_str());
	string config_data;
	char buf[4096];
	while( in ) {
		in.read(buf, sizeof(buf));
		config_data.append(buf, in.gcount());
	}

	return config_data;
}

void OpenSync40::SetConfiguration(const std::string &group_name,
				long member_id,
				const std::string &config_data)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "Member " << member_id << " of group '" << group_name << "' does not accept configuration.";
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


	// To emulate 0.22 behaviour, we need to use 0.4x save-to-file
	// functions, and then load that from the file again, and
	// return that string as the configuratin.

	TempDir tempdir("opensyncapi");

	string filename = tempdir.GetNewFilename();

	// write config data to file
	{
		ofstream out(filename.c_str());
		out << config_data;
	}

	// load brand new config from file
	// if a new config object isn't created here, the loaded config
	// will be added to the existing config
	OSyncPluginConfig *new_config = m_priv->osync_plugin_config_new(m_priv->error);
	if( !m_priv->osync_plugin_config_file_load(new_config, filename.c_str(), m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	m_priv->osync_member_set_config(member, new_config);
	
	if( !m_priv->osync_member_save(member, m_priv->error))
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}

void OpenSync40::Discover(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	EngineHandle engine(m_priv->osync_engine_unref);
	engine = m_priv->osync_engine_new(group, m_priv->error);
	if( !engine.get() )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	SyncListHandle members(m_priv->osync_list_free);
	members = m_priv->osync_group_get_members(group);
	OSyncList *m = NULL;
	for( m = members.get(); m; m = m->next ) {
		OSyncMember *member = (OSyncMember *) m->data;

		/* Discover the objtypes for the members */
		if( !m_priv->osync_engine_discover_and_block(engine.get(), member, m_priv->error))
			break;

		SyncListHandle objtypes(m_priv->osync_list_free);
		objtypes = m_priv->osync_member_get_objtypes(member);
		if( m_priv->osync_list_length(objtypes.get()) == 0 ) {
			m_priv->osync_error_set(m_priv->error, OSYNC_ERROR_GENERIC, "discover failed: no objtypes returned");
			break;
		}

		if( !m_priv->osync_member_save(member, m_priv->error) )
			break;
	}

	// check for error
	if( m ) {
		m_priv->osync_engine_finalize(engine.get(), m_priv->error);
		throw std::runtime_error(m_priv->error.GetErrorMsg());
	}
}

void OpenSync40::Sync(const std::string &group_name,
			SyncStatus &status_callback)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	EngineHandle engine(m_priv->osync_engine_unref);
	engine = m_priv->osync_engine_new(group, m_priv->error);
	if( !engine.get() )
		throw std::runtime_error(m_priv->error.GetErrorMsg());


	CallbackBundle cbdata(m_priv, status_callback);

	m_priv->osync_engine_set_conflict_callback(engine.get(), conflict_handler, &cbdata);
	m_priv->osync_engine_set_changestatus_callback(engine.get(), entry_status, &cbdata);
	m_priv->osync_engine_set_mappingstatus_callback(engine.get(), mapping_status, &cbdata);
	m_priv->osync_engine_set_enginestatus_callback(engine.get(), engine_status, &cbdata);
	m_priv->osync_engine_set_memberstatus_callback(engine.get(), member_status, &cbdata);
	m_priv->osync_engine_set_multiply_callback(engine.get(), multiply_summary, &cbdata);


	SyncListHandle members(m_priv->osync_list_free);
	members = m_priv->osync_group_get_members(group);
	OSyncList *m = NULL;
	for( m = members.get(); m; m = m->next ) {
		OSyncMember *member = (OSyncMember *) m->data;

		SyncListHandle objtypes(m_priv->osync_list_free);
		objtypes = m_priv->osync_member_get_objtypes(member);
		if( m_priv->osync_list_length(objtypes.get()) == 0 ) {
			cout << "Member " << m_priv->osync_member_get_id(member) << " has no objtypes. Has it already been discovered?" << endl;
		}
	}

	if( !m_priv->osync_engine_initialize(engine.get(), m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());

	if( !m_priv->osync_engine_synchronize_and_block(engine.get(), m_priv->error) ) {
		m_priv->osync_engine_finalize(engine.get(), NULL);
		throw std::runtime_error(m_priv->error.GetErrorMsg());
	}

	if( !m_priv->osync_engine_finalize(engine.get(), m_priv->error) )
		throw std::runtime_error(m_priv->error.GetErrorMsg());
}


/////////////////////////////////////////////////////////////////////////////
// TossError public members

/// Returns NULL if no error
const char* TossError::GetErrorMsg()
{
	return m_priv->osync_error_print(&m_error);
}

bool TossError::IsSet()
{
	return m_priv->osync_error_is_set(&m_error);
}

void TossError::Clear()
{
	if( m_error ) {
		m_priv->osync_error_unref(&m_error);
		m_error = 0;
	}
}

} // namespace OpenSync

/////////////////////////////////////////////////////////////////////////////
// osynctool source

#if 0



#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <glib.h>
#include <errno.h>
#include <getopt.h>

#include "config.h"

osync_bool manual = FALSE;
osync_bool always_accept_forecast = FALSE;
OSyncConflictResolution conflict = OSYNC_CONFLICT_RESOLUTION_UNKNOWN;
int winner = 0;

static void short_usage(char *name, int ecode)
{
	fprintf (stderr, "Usage: %s ACTION [OPTIONS]..\n", name);
	fprintf (stderr, "Try `%s --help' for more information\n", name);
	exit(ecode);
}

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s ACTION [OPTIONS]..\n", name);
  fprintf (stderr, "Information about installation:\n");
  fprintf (stderr, "--listplugins    Lists all plugins\n");
  fprintf (stderr, "--listformats    Lists all formats\n");
  fprintf (stderr, "--version        Shows the version of opensync and osynctool\n");
  fprintf (stderr, "--help           Show this help\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Information about configured groups:\n");
  fprintf (stderr, "--listgroups\n");
  fprintf (stderr, "    Lists all groups\n");
  fprintf (stderr, "--showgroup <groupname>\n");
  fprintf (stderr, "    Lists all members of the group\n");
  fprintf (stderr, "--showobjtypes <groupname>\n");
  fprintf (stderr, "    Lists all objtypes that the group can synchronize\n");
  fprintf (stderr, "--showfilter <groupname>\n");
  fprintf (stderr, "    Lists the filters for a group\n");
  fprintf (stderr, "--showcapabilities <groupname>\n");
  fprintf (stderr, "    List all capabilities of the group members\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Group configuration:\n");
  fprintf (stderr, "--addgroup <groupname>\n");
  fprintf (stderr, "    Add a new group\n");
  fprintf (stderr, "--delgroup <groupname>\n");
  fprintf (stderr, "    Delete the given group\n");
  fprintf (stderr, "--configure-filter <groupname>\n");
  fprintf (stderr, "    Configures the filters of a group\n");
  fprintf (stderr, "--enable-objtype <groupname> <objtype>\n");
  fprintf (stderr, "    Enables the synchronization of a objtype for a group\n");
  fprintf (stderr, "--disable-objtype <groupname> <objtype>\n");
  fprintf (stderr, "    Disables the synchronization of a objtype for a group\n");
  fprintf (stderr, "--enable-merger <groupname>\n");
  fprintf (stderr, "    Enable the merger of a group (default: enabled)\n");
  fprintf (stderr, "--disable-merger <groupname>\n");
  fprintf (stderr, "    Disable the merger of a group\n");
  fprintf (stderr, "--enable-converter <groupname>\n");
  fprintf (stderr, "    Enable the converter of a group (default: enabled)\n");
  fprintf (stderr, "--disable-converter <groupname>\n");
  fprintf (stderr, "    Disable the converter of a group (Recommended for backups)\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Group member configuration:\n");
  fprintf (stderr, "--addmember <groupname> <plugin> [member name]\n");
  fprintf (stderr, "    Add a member to the group\n");
  fprintf (stderr, "--delmember <groupname> <memberid>\n");
  fprintf (stderr, "    Delete a member from the group\n");
  fprintf (stderr, "--configure <groupname> <memberid>\n");
  fprintf (stderr, "    Configure a member. memberid as returned by --showgroup\n");
  fprintf (stderr, "--configure-capabilities <groupname> <memberid>\n");
  fprintf (stderr, "    Configures the capabilities of a member from the group\n");
  fprintf (stderr, "--discover <groupname> [<memberid>]\n");
  fprintf (stderr, "    Detect which objtypes are supported by one specified or all members\n");
  fprintf (stderr, "--disable-readonly <groupname> <memberid> [<objtype>]\n");
  fprintf (stderr, "    Enable writing (of objtype sink) for a member (default)\n");
  fprintf (stderr, "--enable-readonly <groupname> <memberid> [<objtype>]\n");
  fprintf (stderr, "    Disable writing (of objtype sink) for a member\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Synchronization:\n");
  fprintf (stderr, "--sync <groupname>\n");
  fprintf (stderr, "    Sync all members in a group\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "    Synchronization options:\n");
  fprintf (stderr, "        [--wait]\n");
  fprintf (stderr, "            Wait for a client to initialize the sync instead of starting immediately\n");
  fprintf (stderr, "        [--multi]\n");
  fprintf (stderr, "            Repeat to wait for sync alerts\n");
  fprintf (stderr, "        [--slow-sync <objtype>]\n");
  fprintf (stderr, "            Perform a slow-sync of all members in the group\n");
  fprintf (stderr, "        [--manual]\n");
  fprintf (stderr, "            Make manual engine iterations. Only for debugging.\n");
  fprintf (stderr, "        [--configdir]\n");
  fprintf (stderr, "            Set a different configuration directory than ~/.opensync\n");
  fprintf (stderr, "        [--conflict 1-9/d/i/n] \n");
  fprintf (stderr, "            Resolve all conflicts as side [1-9] wins, [d]uplicate, [i]gnore, or\n");
  fprintf (stderr, "            keep [n]ewer\n");
  fprintf (stderr, "        [--always-accept-forecast]\n");
  fprintf (stderr, "            Always accept the Synchronization Forecast\n");
  exit (ecode);
}

typedef enum  {
	NONE,
	OSYNCTOOL_INSTALL_LISTPLUGINS,
	OSYNCTOOL_INSTALL_LISTFORMATS,
	OSYNCTOOL_INSTALL_GETVERSION,
	OSYNCTOOL_SHOW_GROUPS,
	OSYNCTOOL_SHOW_GROUP,
	OSYNCTOOL_SHOW_OBJTYPES,
	OSYNCTOOL_SHOW_FILTER,
	OSYNCTOOL_SHOW_CAPABILITIES,
	OSYNCTOOL_CONFIGURE_ADDGROUP,
	OSYNCTOOL_CONFIGURE_DELGROUP,
	OSYNCTOOL_CONFIGURE_ADDMEMBER,
	OSYNCTOOL_CONFIGURE,
	OSYNCTOOL_CONFIGURE_DELMEMBER,
	OSYNCTOOL_CONFIGURE_ENABLE_OBJTYPE,
	OSYNCTOOL_CONFIGURE_DISABLE_OBJTYPE,
	OSYNCTOOL_CONFIGURE_ENABLE_MERGER,
	OSYNCTOOL_CONFIGURE_DISABLE_MERGER,
	OSYNCTOOL_CONFIGURE_ENABLE_CONVERTER,
	OSYNCTOOL_CONFIGURE_DISABLE_CONVERTER,
	OSYNCTOOL_CONFIGURE_FILTER,
	OSYNCTOOL_CONFIGURE_DISCOVER,
	OSYNCTOOL_CONFIGURE_MEMBER_ENABLE_WRITE,
	OSYNCTOOL_CONFIGURE_MEMBER_DISABLE_WRITE,
	OSYNCTOOL_SYNC
} ToolAction;


static osync_bool osynctool_list_formats(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncList *o, *objformats = osync_format_env_get_objformats(env);

	for (o = objformats; o; o = o->next) {
		OSyncObjFormat *format = (OSyncObjFormat *) o->data;
		printf("Format: %s\n", osync_objformat_get_name(format));
		printf("\tObject Type: %s\n", osync_objformat_get_objtype(format));
	}

	osync_list_free(objformats);

	return TRUE;
}

static void osynctool_list_plugins(OSyncPluginEnv *env)
{
	OSyncPlugin *plugin;
	OSyncList *plugins, *p;
	
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	
	printf("Available plugins:\n");
	
	plugins = osync_plugin_env_get_plugins(env);
	for (p = plugins; p; p = p->next) {
		plugin = (OSyncPlugin *) p->data;
		printf("%s\n", osync_plugin_get_name(plugin));
	}
	osync_list_free(plugins);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void osynctool_version(void)
{
	printf("This is osynctool version \"%s\"\n", OSYNCTOOL_VERSION);
	printf("using OpenSync version \"%s\"\n", osync_get_version());
}

static void osynctool_show_groups(OSyncGroupEnv *env)
{
	OSyncGroup *group;
	OSyncList *g, *groups =  osync_group_env_get_groups(env);
	
	printf("Available groups:\n");
	
	for (g = groups; g; g = g->next) {
		group = (OSyncGroup *) g->data;
		printf("%s\n", osync_group_get_name(group));
	}

	osync_list_free(groups);
}

static const char *OSyncChangeType2String(OSyncChangeType type)
{
	switch (type) {
		case OSYNC_CHANGE_TYPE_ADDED: return "ADDED";
		case OSYNC_CHANGE_TYPE_UNMODIFIED: return "UNMODIFIED";
		case OSYNC_CHANGE_TYPE_DELETED: return "DELETED";
		case OSYNC_CHANGE_TYPE_MODIFIED: return "MODIFIED";
		case OSYNC_CHANGE_TYPE_UNKNOWN: return "?";
	}
	return NULL;
}

static void member_status(OSyncEngineMemberUpdate *status, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, status, user_data);

	OSyncMember *member = osync_engine_member_update_get_member(status);
	const char *objtype = osync_engine_member_update_get_objtype(status);
	OSyncError *error = osync_engine_member_update_get_error(status);
	
	char *sink = NULL;
	if (objtype == NULL) {
		sink = g_strdup("Main sink");
	} else {
		sink = g_strdup_printf("%s sink", objtype);
	}
	
	switch (osync_engine_member_update_get_event(status)) {
		case OSYNC_ENGINE_MEMBER_EVENT_CONNECTED:
			printf("%s of member %lli of type %s just connected\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member));
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_CONNECT_DONE:
			/* Special event - but not interesting for the normal user. */
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_DISCONNECTED:
			printf("%s of member %lli of type %s just disconnected\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member));
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_READ:
			printf("%s of member %lli of type %s just sent all changes\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member));
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_WRITTEN:
			printf("%s of member %lli of type %s committed all changes.\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member));
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_SYNC_DONE:
			printf("%s of member %lli of type %s reported sync done.\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member));
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_DISCOVERED:
			printf("%s of member %lli of type %s discovered its objtypes.\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member));
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_ERROR:
			printf("%s of member %lli of type %s had an error: %s\n", sink, osync_member_get_id(member), osync_member_get_pluginname(member), osync_error_print(&error));
			break;
	}
	
	g_free(sink);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void multiply_summary(OSyncEngine *engine, void *user_data)
{
	const OSyncList *mapping_entry_engines, *e;
	osync_bool dirty = FALSE;
	OSyncError *error = NULL;
	OSyncList *o, *objengines = osync_engine_get_objengines(engine);
	OSyncList *m, *members;

	osync_trace(TRACE_ENTRY, "%s(%p %p)", __func__, engine, user_data);
	
	printf("\nSynchronization Forecast Summary:\n");

	for (o = objengines; o; o = o->next) {
		OSyncChangeType type;
		OSyncObjEngine *objengine = (OSyncObjEngine *) o->data;
		unsigned int added, modified, deleted;



		printf("\nObjType: %s\n", osync_obj_engine_get_objtype(objengine));

		members = osync_obj_engine_get_members(objengine);
		for (m = members; m; m = m->next) {
			OSyncMember *member = (OSyncMember *) m->data;

			mapping_entry_engines = osync_obj_engine_get_mapping_entry_engines_of_member(objengine, member);

			added = modified = deleted = 0;

			for (e = mapping_entry_engines; e; e = e->next) {
				OSyncMappingEntryEngine *entry_engine = e->data;


				if (!osync_entry_engine_is_dirty(entry_engine))
					continue;

				dirty = TRUE;

				type = osync_entry_engine_get_changetype(entry_engine);
				switch (type) {
					case OSYNC_CHANGE_TYPE_ADDED:
						added++;
						break;
					case OSYNC_CHANGE_TYPE_MODIFIED:
						modified++;
						break;
					case OSYNC_CHANGE_TYPE_DELETED:
						deleted++;
						break;
					default:
						break;
				}
			}

			printf("\tMember %lli: Adding(%u) Modifying(%u) Deleting(%u)\n",
					osync_member_get_id(member),
					added, modified, deleted);


		}
	}

	printf("\n");

	/* Always accept the forecast */
	if (dirty && always_accept_forecast) {
		if (!osync_engine_continue(engine, &error))
			goto error;
	/* Ask for forecast acceptance */
	} else if (dirty) {
		printf("Do you want to continue the synchronization? (N/y)");
		printf(": ");
		fflush(stdout);
		int inp = getchar();
		while (inp != '\n' && getchar() != '\n');

		/* Abort if not got accepted with 'y' */
		if (inp != 'y') {
			if (!osync_engine_abort(engine, &error))
				goto error;

			printf("\nAborting! Synchronization got aborted by user!\n");
		} else {
			if (!osync_engine_continue(engine, &error))
				goto error;

			printf("\nOK! Completing synchronization!\n");
		}

	} else {
		printf("No modifications of any Member. Continuing.\n");
		if (!osync_engine_continue(engine, &error))
			goto error;
	}


	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	fprintf(stderr, "ERROR: %s", osync_error_print(&error));
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return;
}

static void entry_status(OSyncEngineChangeUpdate *status, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, status, user_data);

	OSyncChange *change = osync_engine_change_update_get_change(status);
	OSyncMember *member = osync_engine_change_update_get_member(status);
	OSyncError *error = osync_engine_change_update_get_error(status);
	
	switch (osync_engine_change_update_get_event(status)) {
		case OSYNC_ENGINE_CHANGE_EVENT_READ:
			printf("Received an entry %s (%s) from member %lli (%s). Changetype %s\n",
					osync_change_get_uid(change),
					osync_objformat_get_name(osync_change_get_objformat(change)),
					osync_member_get_id(member),
					osync_member_get_pluginname(member),
					OSyncChangeType2String(osync_change_get_changetype(change)));
			break;
		case OSYNC_ENGINE_CHANGE_EVENT_WRITTEN:
			printf("Sent an entry %s (%s) to member %lli (%s). Changetype %s\n",
					osync_change_get_uid(change),
					osync_objformat_get_name(osync_change_get_objformat(change)),
					osync_member_get_id(member),
					osync_member_get_pluginname(member),
					OSyncChangeType2String(osync_change_get_changetype(change)));
			break;
		case OSYNC_ENGINE_CHANGE_EVENT_ERROR:
			printf("Error for entry %s (%s) and member %lli (%s): %s\n",
					osync_change_get_uid(change),
					osync_objformat_get_name(osync_change_get_objformat(change)),
					osync_member_get_id(member),
					osync_member_get_pluginname(member),
					osync_error_print(&(error)));
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void mapping_status(OSyncEngineMappingUpdate *status, void *user_data)
{

	OSyncError *error = osync_engine_mapping_update_get_error(status);

	switch (osync_engine_mapping_update_get_event(status)) {
		case OSYNC_ENGINE_MAPPING_EVENT_SOLVED:
			/*printf("Mapping solved\n");*/
			break;
		case OSYNC_ENGINE_MAPPING_EVENT_ERROR:
			printf("Mapping Error: %s\n", osync_error_print(&(error)));
			break;
	}
}

static void engine_status(OSyncEngineUpdate *status, void *user_data)
{
	OSyncError *error = osync_engine_update_get_error(status);

	switch (osync_engine_update_get_event(status)) {
		case OSYNC_ENGINE_EVENT_CONNECTED:
			printf("All clients connected or error\n");
			break;
		case OSYNC_ENGINE_EVENT_CONNECT_DONE:
			/* Not of interest for regular user. */
			break;
		case OSYNC_ENGINE_EVENT_READ:
			printf("All clients sent changes or error\n");
			break;
		case OSYNC_ENGINE_EVENT_MAPPED:
			printf("All changes got mapped\n");
			break;
		case OSYNC_ENGINE_EVENT_MULTIPLIED:
			printf("All changes got multiplied\n");
			break;
		case OSYNC_ENGINE_EVENT_PREPARED_WRITE:
			printf("All changes got prepared for write\n");
			break;
		case OSYNC_ENGINE_EVENT_PREPARED_MAP:
			/* Not of interest for regular user. */
			break;
		case OSYNC_ENGINE_EVENT_WRITTEN:
			printf("All clients have written\n");
			break;
		case OSYNC_ENGINE_EVENT_DISCONNECTED:
			printf("All clients have disconnected\n");
			break;
		case OSYNC_ENGINE_EVENT_ERROR:
			printf("The sync failed: %s\n", osync_error_print(&(error)));
			break;
		case OSYNC_ENGINE_EVENT_SUCCESSFUL:
			printf("The sync was successful\n");
			break;
		case OSYNC_ENGINE_EVENT_PREV_UNCLEAN:
			printf("The previous synchronization was unclean. Slow-syncing\n");
			break;
		case OSYNC_ENGINE_EVENT_END_CONFLICTS:
			printf("All conflicts have been reported\n");
			break;
		case OSYNC_ENGINE_EVENT_SYNC_DONE:
			printf("All clients reported sync done\n");
			break;

	}
}

static void conflict_handler(OSyncEngine *engine, OSyncMappingEngine *mapping, void *user_data)
{
	int i;
	OSyncError	*error = NULL;
	OSyncConflictResolution res = OSYNC_CONFLICT_RESOLUTION_UNKNOWN;
	OSyncChange *change = NULL;
	OSyncList *c, *changes = osync_mapping_engine_get_changes(mapping);
	
	printf("Conflict for Mapping %p: ", mapping);
	fflush(stdout);
	
	if (conflict != OSYNC_CONFLICT_RESOLUTION_UNKNOWN)
		res = conflict;
	
	/* Check if the original value for the winning side was valid : memberid start from 1 thus winner + 1 */
	if (conflict == OSYNC_CONFLICT_RESOLUTION_SELECT) {
		if (!osync_mapping_engine_member_change(mapping, winner + 1)) {
			printf("Unable to find change #%i\n", winner + 1);
			res = OSYNC_CONFLICT_RESOLUTION_UNKNOWN;
		}
	}

	osync_bool supports_ignore = osync_mapping_engine_supports_ignore(mapping);
	osync_bool supports_use_latest = osync_mapping_engine_supports_use_latest(mapping);

	if (res == OSYNC_CONFLICT_RESOLUTION_UNKNOWN) {
		for (i = 0, c = changes; c; c = c->next, i++) {
			OSyncChange *change = (OSyncChange *) c->data;
			if (osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_UNKNOWN) {
				OSyncMember *member = osync_mapping_engine_change_find_member(mapping, change);
				OSyncData *data = osync_change_get_data(change);
				char *printable = osync_data_get_printable(data, &error);
				if ( osync_error_is_set(&error) ) {
					fprintf(stderr, "Problems while acquiring printable format of data: %s\n",
							osync_error_print(&error));
					osync_error_unref(&error);
					return;
				}
				printf("\nEntry %i:\nMember: %lli (%s)\nUID: %s\n%s\n", i+1, 
						osync_member_get_id(member), 
						osync_member_get_pluginname(member),
						osync_change_get_uid(change), 
						printable);
				osync_free(printable);
			}
		}
		
		while (res == OSYNC_CONFLICT_RESOLUTION_UNKNOWN) {
			printf("\nWhich entry do you want to use? [1-9] To select a side, [A]bort, [D]uplicate");
			
			if (supports_ignore)
				printf(", [I]gnore");
			
			if (supports_use_latest)
				printf(", Keep [N]ewer");
			
			printf(": ");
			fflush(stdout);
			int inp = getchar();
			while (inp != '\n' && getchar() != '\n');
			
			if (inp == 'D' || inp == 'd')
				res = OSYNC_CONFLICT_RESOLUTION_DUPLICATE;
			else if (supports_ignore && (inp == 'i' || inp == 'I'))
				res = OSYNC_CONFLICT_RESOLUTION_IGNORE;
			else if (supports_use_latest && (inp == 'n' || inp == 'N'))
				res = OSYNC_CONFLICT_RESOLUTION_NEWER;
			else if (strchr("123456789", inp) != NULL) {
				char inpbuf[2];
				inpbuf[0] = inp;
				inpbuf[1] = 0;
				
				winner = atoi(inpbuf) - 1;
				if (winner >= 0) {
					if (!osync_list_nth(changes, winner))
						printf("Unable to find change #%i\n", winner + 1);
					else
						res = OSYNC_CONFLICT_RESOLUTION_SELECT;
				}
			} else if (inp == 'a' || inp == 'A') {
				if (!osync_engine_abort(engine, &error)) {
					fprintf(stderr, "Problems while aborting: %s\n",
							osync_error_print(&error));
					osync_error_unref(&error);
				}

				return;
			}
		}
	}
		
	/* Did we get default conflict resolution ? */
	switch (res) {
		case OSYNC_CONFLICT_RESOLUTION_DUPLICATE:
			printf("Mapping duplicated\n");
			if (!osync_engine_mapping_duplicate(engine, mapping, &error))
				goto error;
			return;
		case OSYNC_CONFLICT_RESOLUTION_IGNORE:
			printf("Conflict ignored\n");
			if (!osync_engine_mapping_ignore_conflict(engine, mapping, &error))
				goto error;
			return;
		case OSYNC_CONFLICT_RESOLUTION_NEWER:
			printf("Newest entry used\n");
			if (!osync_engine_mapping_use_latest(engine, mapping, &error))
				goto error;
			return;
		case OSYNC_CONFLICT_RESOLUTION_SELECT:
			printf("Solving conflict\n");
			
			c = osync_list_nth(changes, winner);
			osync_assert(c);

			change = (OSyncChange *) c->data;

			if (!osync_engine_mapping_solve(engine, mapping, change, &error))
				goto error;
			return;
		case OSYNC_CONFLICT_RESOLUTION_UNKNOWN:
			g_assert_not_reached();
	}

	osync_list_free(changes);
	
	return;

error:
	osync_list_free(changes);
	printf("Conflict not resolved: %s\n", osync_error_print(&error));
	osync_error_unref(&error);
}

static osync_bool osynctool_synchronize(OSyncGroupEnv *env, char *groupname, osync_bool wait, osync_bool multi, GList *objtypes, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %i, %i, %p, %p)", __func__, env, groupname, wait, multi, objtypes, error);
	
	OSyncList *m, *members;
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		goto error;
	}
	
	printf("Synchronizing group \"%s\" %s\n", osync_group_get_name(group), objtypes ? "[slow sync]" : "");

	OSyncEngine *engine = osync_engine_new(group, error);
	if (!engine)
		goto error;
	
	osync_engine_set_conflict_callback(engine, conflict_handler, NULL);
	osync_engine_set_changestatus_callback(engine, entry_status, NULL);
	osync_engine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	osync_engine_set_multiply_callback(engine, multiply_summary, NULL);
	
	/*osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler, NULL);*/
	//osengine_set_message_callback(engine, plgmsg_function, "palm_uid_mismatch", NULL);
	//osengine_flag_only_info(engine);
	//if (manual)
	//	osengine_flag_manual(engine);

	members = osync_group_get_members(group);
	for (m = members; m; m = m->next) {
		OSyncMember *member = (OSyncMember *) m->data;
		OSyncList *objtypes = osync_member_get_objtypes(member);
		if (osync_list_length(objtypes) == 0) {
			printf("Member %llu has no objtypes. Has it already been discovered?\n", osync_member_get_id(member));
		}
	}
	osync_list_free(members);

	if (!osync_engine_initialize(engine, error))
		goto error_free_engine;
	
	/* Set slowsync for requested object types */
	GList *o = NULL;
	for (o = objtypes; o; o = o->next) {
		char *objtype = (char *) o->data;
		/* osync_engine_find_objengine is only able to find
		   something if the engine is initialized! */
		OSyncObjEngine *objengine = osync_engine_find_objengine(engine, objtype);

		if (!objengine) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find Object Engine for object type  \"%s\"", objtype);
			goto error_finalize;
		}

		osync_obj_engine_set_slowsync(objengine, TRUE);
	}

	do {
		if (!osync_engine_synchronize_and_block(engine, error))
			goto error_finalize;
	} while (multi);
	
	/*if (!wait) {
		if (!manual) {
			if (!osengine_sync_and_block(engine, &error)) {
				printf("Error synchronizing: %s\n", osync_error_print(&error));
				osync_error_free(&error);
				return;
			}
		} else {
			if (!osengine_synchronize(engine, &error)) {
				printf("Error while starting synchronization: %s\n", osync_error_print(&error));
				osync_error_free(&error);
				return;
			}
			char buf[1024];
			while (1) {
				if (fgets(buf, sizeof(buf), stdin) == NULL)
					break;
				printf("+++++++++++++++++++++++++++\nNew Engine iteration:\n");
				osengine_one_iteration(engine);
				osengine_print_all(engine);
			}
		}
	} else {
		do {
			osengine_wait_sync_end(engine, NULL);
		} while (multi);
	}*/

	
	if (!osync_engine_finalize(engine, error))
		goto error_free_engine;
	
	osync_engine_unref(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_finalize:
	osync_engine_finalize(engine, NULL);
error_free_engine:
	osync_engine_unref(engine);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool osynctool_show_group(OSyncGroupEnv *env, const char *groupname, OSyncError **error)
{
	const char *membername = NULL;
	OSyncMember *member = NULL;
	OSyncList *m, *members;
	
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	printf("Group: %s\n", osync_group_get_name(group));
	
	members = osync_group_get_members(group);
	for (m=members; m; m = m->next) {
		member = (OSyncMember *) m->data;
		membername = osync_member_get_name(member);
		if (membername) {
			printf("Member %lli (%s): %s\n", osync_member_get_id(member), membername, osync_member_get_pluginname(member));
		} else {
			printf("Member %lli: %s\n", osync_member_get_id(member), osync_member_get_pluginname(member));
		}

		/*
		 * Print the current configuration
		 */
		OSyncPluginConfig *config = osync_member_get_config(member, error);
		if (!config) {
			printf("\tNo Configuration found: %s\n", osync_error_print(error));
			osync_error_unref(error);
		} else {
			/* TODO: implement print funciton... print fields of OSyncPluginConfig.
			printf("\tConfiguration : %s\n", config);
			*/
		}
	}

	osync_list_free(members);

	return TRUE;
}

static osync_bool osynctool_show_objtypes(OSyncGroupEnv *env, const char *groupname, OSyncError **error)
{
	unsigned int max;
	OSyncList *o, *group_objtypes, *m, *members;
	
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	members = osync_group_get_members(group);
	for (m=members; m; m = m->next) {
		OSyncMember *member = (OSyncMember *) m->data;
		OSyncList *o, *objtypes = osync_member_get_objtypes(member);
		if (osync_list_length(objtypes) == 0) {
			printf("Member %lli has no objtypes. Has it already been discovered?\n", osync_member_get_id(member));
		} else {
			printf("Member %lli Objtypes:\n", osync_member_get_id(member));
			
			for (o=objtypes; o; o = o->next) {
				const char *objtype = (const char *) o->data;
				printf("\tObjtype %s: %s\n", objtype, osync_member_objtype_enabled(member, objtype) ? "Enabled" : "Disabled");
				const OSyncList *formats = osync_member_get_objformats(member, objtype, error);
				if (!formats) {
					printf("\t\tNo formats found: %s\n", osync_error_print(error));
					osync_error_unref(error);
				}
				
				for (; formats; formats = formats->next) {
					OSyncObjFormatSink *formatsink = formats->data;
					const char * conversionconfig = osync_objformat_sink_get_config(formatsink);
					if (conversionconfig != NULL) {
						printf("\t\t\t\t conversion config: %s\n", conversionconfig);
					}
					else {
						printf("\t\t\t\t conversion config: not configured\n");
					}
				}
			}
		}
		osync_list_free(objtypes);
	}
	osync_list_free(members);
	
	group_objtypes = osync_group_get_objtypes(group);
	max = osync_list_length(group_objtypes);
	if (max == 0) {
		printf("Group has no objtypes. Have the objtypes already been discovered?\n");
	} else {
		printf("Objtypes for the group:\n");
		for (o=group_objtypes; o; o = o->next) {
			const char *objtype = (const char *) o->data;
			printf("\t%s: %s\n", objtype, osync_group_objtype_enabled(group, objtype) ? "Enabled" : "Disabled");
		}
	}
	osync_list_free(group_objtypes);
	
	return TRUE;
}

static osync_bool osynctool_show_filter(OSyncGroupEnv *env, const char *groupname, OSyncError **error)
{
	return TRUE;
}

static osync_bool osynctool_show_capabilities(OSyncGroupEnv *env, const char *groupname, OSyncError **error)
{
	OSyncMember *member = NULL;
	OSyncList *m, *members, *o, *objtypes;
	
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	printf("Group: %s\n", osync_group_get_name(group));
	
	members = osync_group_get_members(group);
	for (m=members; m; m = m->next) {
		member = (OSyncMember *) m->data;
		printf("Member %lli: %s\n", osync_member_get_id(member), osync_member_get_pluginname(member));

		OSyncCapabilities *capabilities = osync_member_get_capabilities(member);
		if (!capabilities) {
			printf("No capabilities found.\n");
			continue;
		}

		objtypes = osync_member_get_objtypes(member);
		for (o=objtypes; o; o = o->next) {
			const char *objtype = (const char *) o->data;
			OSyncCapabilitiesObjType *capsobjtype = osync_capabilities_get_objtype(capabilities, objtype);
			OSyncList *caps = osync_capabilities_objtype_get_caps(capsobjtype);
			printf("Capabilities for \"%s\":\n", objtype);
			printf("Name\tDisplayName\tMaxOccurs\tMax\tMin\n");
			for (; caps; caps = caps->next) {
				OSyncCapability *cap = (OSyncCapability *) caps->data;
				printf("%s\t%s\t%u\t%u\t%u\n",
						osync_capability_get_name(cap),
						osync_capability_get_displayname(cap),
						osync_capability_get_maxoccurs(cap),
						osync_capability_get_max(cap),
						osync_capability_get_min(cap));
			}
		}
	}
	osync_list_free(members);

	return TRUE;
}

static osync_bool osynctool_add_group(OSyncGroupEnv *env, char *groupname, OSyncError **error)
{
	OSyncGroup *group = osync_group_new(error);
	if (!group)
		goto error;
	
	osync_group_set_name(group, groupname);
	if (!osync_group_env_add_group(env, group, error))
		goto error_and_free;
	
	if (!osync_group_save(group, error))
		goto error_and_free;
	
	osync_group_unref(group);
	return TRUE;

error_and_free:
	osync_group_unref(group);
error:
	return FALSE;
}

static osync_bool osynctool_del_group(OSyncGroupEnv *env, char *groupname, OSyncError **error)
{
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	if (!osync_group_delete(group, error))
		return FALSE;
	
	osync_group_env_remove_group(env, group);
	
	return TRUE;
}

static osync_bool osynctool_add_member(OSyncGroupEnv *env, OSyncPluginEnv *plugin_env, const char *groupname, const char *pluginname, const char *membername, OSyncError **error)
{
	OSyncMember *member = NULL;
	OSyncPlugin *plugin = NULL;
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	plugin = osync_plugin_env_find_plugin(plugin_env, pluginname);
	if (!plugin) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plugin with name %s", pluginname);
		return FALSE;
	}
	
	member = osync_member_new(error);
	if (!member) 
		return FALSE;

	osync_group_add_member(group, member);
	osync_member_set_pluginname(member, pluginname);

	if (membername)
		osync_member_set_name(member, membername);
	
	if (!osync_member_save(member, error)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to save member: %s", osync_error_print(error));
		return FALSE;
	}
	
	osync_member_unref(member);
	
	return TRUE;
}

static osync_bool osynctool_del_member(OSyncGroupEnv *env, const char *groupname, const char *memberid, OSyncError **error)
{
	long long id = 0;
	OSyncMember *member = NULL;
	
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	id = atoi(memberid);
	member = osync_group_find_member(group, id);
	if (!member) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find member with id %s", memberid);
		return FALSE;
	}
	
	if (!osync_member_delete(member, error))
		return FALSE;
	
	osync_group_remove_member(group, member);

	return TRUE;
}

static osync_bool osynctool_enable_objtype(OSyncGroupEnv *env, const char *groupname, const char *objtype, osync_bool enable, OSyncError **error)
{
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	osync_group_set_objtype_enabled(group, objtype, enable);
	
	if (!osync_group_save(group, error))
		return FALSE;
	
	return TRUE;
}

static osync_bool osynctool_configure_group_set_merger_enabled(OSyncGroupEnv *env, const char *groupname, osync_bool enable, OSyncError **error)
{
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	osync_group_set_merger_enabled(group, enable);
	
	if (!osync_group_save(group, error))
		return FALSE;
	
	return TRUE;
}

static osync_bool osynctool_configure_group_set_converter_enabled(OSyncGroupEnv *env, const char *groupname, osync_bool enable, OSyncError **error)
{
	OSyncGroup *group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		return FALSE;
	}
	
	osync_group_set_converter_enabled(group, enable);
	
	if (!osync_group_save(group, error))
		return FALSE;
	
	return TRUE;
}

static osync_bool edit_config(OSyncPluginConfig **config, OSyncError** error)
{
	int file = 0;
	char *tmpfile = NULL;
	char *editcmd = NULL;
	char *editor = NULL;
	
	tmpfile = g_strdup_printf("%s/osynctooltmp-XXXXXX", g_get_tmp_dir());
	file = g_mkstemp(tmpfile);
	if (!file) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create temp file");
		goto error_free_tmp;
	}
	
	if (*config) {
		if(!osync_plugin_config_file_save(*config, tmpfile, error)) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to write to temp file: %i", errno);
			goto error_close_file;
		}
	}

	if (close(file) == -1)  {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to close temp file: %i", errno);
		goto error_free_tmp;
	}
	
#ifdef _WIN32
	editcmd = g_strdup_printf("notepad %s", tmpfile);
#else
	if (!(editor = getenv("EDITOR")))
		editor = getenv("VISUAL");
	
	if (editor)
		editcmd = g_strdup_printf("%s %s", editor, tmpfile);
	else	
		editcmd = g_strdup_printf("vi %s", tmpfile);
#endif

	if (system(editcmd)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open editor. Aborting");
		g_free(editcmd);
		goto error_free_tmp;
	}

	g_free(editcmd);
	*config = osync_plugin_config_new(error); /* if a new config isn't created the loaded config is added to the old config */
	if (!osync_plugin_config_file_load(*config, tmpfile, error)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to load config from tmpfile %s. Aborting", tmpfile);
		goto error_free_tmp;
	}
	
	if (remove(tmpfile) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to remove file %s", tmpfile);
		goto error_free_tmp;
	}
	
	g_free(tmpfile);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_close_file:
	close(file);
error_free_tmp:
	g_free(tmpfile);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;	
}

static osync_bool osynctool_configure_member(OSyncGroupEnv *env, OSyncPluginEnv *pluginenv, const char *groupname, const char *memberid, OSyncError **error)
{
	long long id = 0;
	OSyncMember *member = NULL;
	OSyncGroup *group = NULL;
	OSyncPlugin *plugin = NULL;
	OSyncPluginConfig *config = NULL;
	OSyncPluginConfigurationType type = OSYNC_PLUGIN_NO_CONFIGURATION;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %s, %p)", __func__, env, pluginenv, groupname, memberid, error);
	
	group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		goto error;
	}
	
	id = atoi(memberid);
	member = osync_group_find_member(group, id);
	if (!member) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find member with id %s", memberid);
		goto error;
	}
	
	plugin = osync_plugin_env_find_plugin(pluginenv, osync_member_get_pluginname(member));
	if (!plugin) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plugin with name %s", osync_member_get_pluginname(member));
		return FALSE;
	}
	
	type = osync_plugin_get_config_type(plugin);
	if (type == OSYNC_PLUGIN_NO_CONFIGURATION) {
		printf("This plugin has no options and does not need to be configured\n");
		osync_trace(TRACE_EXIT, "%s: No options", __func__);
		return TRUE;
	}
	
	config = osync_member_get_config_or_default(member, error);
	if (!config)
		goto error;

	if (!edit_config(&config, error))
		goto error;
	
	osync_member_set_config(member, config);
	
	if (!osync_member_save(member, error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool osynctool_configure_member_sink(OSyncGroupEnv *env, const char *groupname, const char *memberid, const char *objtype, osync_bool isEnable, OSyncError **error)
{
	long long id = 0;
	OSyncMember *member = NULL;
	OSyncGroup *group = NULL;
	OSyncList *objtypes = NULL, *o;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %s, %i, %p)", __func__, env, groupname, memberid, objtype, isEnable, error);
	
	group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		goto error;
	}
	
	id = atoi(memberid);
	member = osync_group_find_member(group, id);
	if (!member) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find member with id %s", memberid);
		goto error;
	}

	/* If certain objtype is to be configured, just configure this one. */ 
	if (objtype) {
		OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
		osync_objtype_sink_set_write(sink, isEnable);
	} else {
		/* Otherwise configure all sinks */
		objtypes = osync_member_get_objtypes(member);
		if (osync_list_length(objtypes) == 0) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Member %lli has no objtypes. Has it already been discovered?\n", osync_member_get_id(member));
			goto error;
		} else {
			for (o = objtypes; o; o = o->next) {
				const char *objtype = (const char*) o->data;
				OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
				osync_objtype_sink_set_write(sink, isEnable);
			}
		}
	}
	osync_list_free(objtypes);
	
	if (!osync_member_save(member, error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_list_free(objtypes);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool osynctool_configure_filter(OSyncGroupEnv *env, const char *groupname, OSyncError **error)
{
	return TRUE;
}

static osync_bool osynctool_configure_discover_member(OSyncEngine *engine, OSyncMember *member, OSyncError **error)
{
	OSyncList *objtypes, *o;

	/* Discover the objtypes for the members */
	if (!osync_engine_discover_and_block(engine, member, error))
		return FALSE;

	objtypes = osync_member_get_objtypes(member);
	if (osync_list_length(objtypes) == 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "discover failed: no objtypes returned");
		return FALSE;
	} else {
		printf("Discovered Objtypes:\n");
				
		for (o = objtypes; o; o = o->next) {
			const char *objtype = (const char *) o->data;
			printf("\t%s\n", objtype);
			const OSyncList *formats = osync_member_get_objformats(member, objtype, error);
			if (!formats) {
				printf("\t\tNo formats found: %s\n", osync_error_print(error));
				osync_error_unref(error);
			}
					
			for (; formats; formats = formats->next) {
				OSyncObjFormatSink *format_sink = formats->data;
				const char *objformat = osync_objformat_sink_get_objformat(format_sink);
				const char *config = osync_objformat_sink_get_config(format_sink);

				printf("\t\tFormat: %s\n", objformat);
				if (config)
					printf("\t\t\t\t conversion config: %s\n", config);
			}
		}
	}

	osync_list_free(objtypes);

	if (!osync_member_save(member, error))
		return FALSE;

	return TRUE;
}

static osync_bool osynctool_configure_discover(OSyncGroupEnv *env, OSyncPluginEnv *pluginenv, const char *groupname, const char *memberid, OSyncError **error)
{
	long long id = 0;
	OSyncMember *member = NULL;
	OSyncGroup *group = NULL;
	OSyncList *members = NULL, *m;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %s, %p)", __func__, env, pluginenv, groupname, memberid, error);
	
	group = osync_group_env_find_group(env, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		goto error;
	}

	OSyncEngine *engine = osync_engine_new(group, error);
	if (!engine)
		goto error;
	
	if (memberid) {
		id = atoi(memberid);
		member = osync_group_find_member(group, id);
		if (!member) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find member with id %s", memberid);
			goto error;
		}

		if (!osynctool_configure_discover_member(engine, member, error))
			goto error_engine_finalize;
	} else {
		members = osync_group_get_members(group);
		for (m = members; m; m = m->next) {
			member = (OSyncMember *) m->data;
			if (!osynctool_configure_discover_member(engine, member, error))
				goto error_engine_finalize;
		}
		osync_list_free(members);
	}

	//osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	//osync_engine_set_memberstatus_callback(engine, member_status, NULL);
	
	osync_engine_unref(engine);


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_engine_finalize:
	osync_engine_finalize(engine, error);
	osync_engine_unref(engine);
error:
	osync_list_free(members);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

int main (int argc, char *argv[])
{
	OSyncError *error = NULL;
	char *groupname = NULL;
	char *membername = NULL;
	char *pluginname = NULL;
	ToolAction action = NONE;
	osync_bool wait = FALSE;
	osync_bool multi = FALSE;
	char *configdir = NULL;
	GList *slow_objects = NULL;
	char *objtype = NULL;
	OSyncGroupEnv *group_env = NULL;
	OSyncFormatEnv *format_env = NULL;
	OSyncPluginEnv *plugin_env = NULL;
	manual = FALSE;
	
	osync_trace(TRACE_ENTRY, "%s(%i, %p)", __func__, argc, argv);
	
	int c;
	int option_index = 0;
	int action_count = 0;

	struct option long_options[] = {
		{"listplugins", no_argument, (int *)&action, OSYNCTOOL_INSTALL_LISTPLUGINS},
		{"listformats", no_argument, (int *)&action, OSYNCTOOL_INSTALL_LISTFORMATS},
		{"version", no_argument, (int *)&action, OSYNCTOOL_INSTALL_GETVERSION},
		{"listgroups", no_argument, (int *)&action, OSYNCTOOL_SHOW_GROUPS},
		{"showgroup", no_argument, (int *)&action, OSYNCTOOL_SHOW_GROUP},
		{"showobjtypes", no_argument, (int *)&action, OSYNCTOOL_SHOW_OBJTYPES},
		{"showfilter", no_argument, (int *)&action, OSYNCTOOL_SHOW_FILTER},
		{"showcapabilities", no_argument, (int *)&action, OSYNCTOOL_SHOW_CAPABILITIES},
		{"addgroup", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_ADDGROUP},
		{"delgroup", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_DELGROUP},
		{"enable-merger", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_ENABLE_MERGER},
		{"disable-merger", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_DISABLE_MERGER},
		{"enable-converter", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_ENABLE_CONVERTER},
		{"disable-converter", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_DISABLE_CONVERTER},
		{"configure-filter", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_FILTER},
		{"addmember", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_ADDMEMBER},
		{"configure", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE},
		{"delmember", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_DELMEMBER},
		{"enable-objtype", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_ENABLE_OBJTYPE},
		{"disable-objtype", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_DISABLE_OBJTYPE},
		{"disable-readonly", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_MEMBER_ENABLE_WRITE},
		{"enable-readonly", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_MEMBER_ENABLE_WRITE},
		{"discover", no_argument, (int *)&action, OSYNCTOOL_CONFIGURE_DISCOVER},
		{"sync", no_argument, (int *)&action, OSYNCTOOL_SYNC},
		{"slow-sync", required_argument, 0, 's'},
		{"wait", no_argument, &wait, TRUE},
		{"always-accept-forecast", no_argument, &always_accept_forecast, TRUE},
		{"multi", no_argument, &multi, TRUE},
		{"conflict", required_argument, 0, 'c'},
		{"configdir", required_argument, 0, 'd'},
		{"manual", no_argument, &manual, TRUE},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	
	while (1) {
		c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {

		case 0: /* flag has been set */
			if (long_options[option_index].flag == (int *)&action) {
				if (action_count > 0) {
					fprintf(stderr, "Error: Only one action may be given at a time\n");
					short_usage(argv[0], 1);
				}
				++action_count;
			}
			break;
		case 's':
			slow_objects = g_list_append(slow_objects, optarg);
			break;
		case 'c':
			if (optarg[0] == 'd' || optarg[0] == 'D')
				conflict = OSYNC_CONFLICT_RESOLUTION_DUPLICATE;
			else if (optarg[0] == 'i' || optarg[0] == 'I')
				conflict = OSYNC_CONFLICT_RESOLUTION_IGNORE;
			else if (optarg[0] == 'n' || optarg[0] == 'N')
				conflict = OSYNC_CONFLICT_RESOLUTION_NEWER;
			else if (strchr("123456789", optarg[0]) != NULL) {
				winner = atoi(optarg) - 1;
				if (winner < 0)
					usage (argv[0], 1);
				conflict = OSYNC_CONFLICT_RESOLUTION_SELECT;
			} else
				usage (argv[0], 1);
			break;
		case 'd':
			configdir = optarg;
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		case '?':
		default:
			short_usage(argv[0], 1);
		}
	}

	switch (action) {
		case NONE:
			short_usage(argv[0], 1);
			break;
		case OSYNCTOOL_SHOW_GROUP:
		case OSYNCTOOL_SHOW_OBJTYPES:
		case OSYNCTOOL_SHOW_FILTER:
		case OSYNCTOOL_SHOW_CAPABILITIES:
		case OSYNCTOOL_CONFIGURE_ADDGROUP:
		case OSYNCTOOL_CONFIGURE_DELGROUP:
		case OSYNCTOOL_CONFIGURE_ENABLE_MERGER:
		case OSYNCTOOL_CONFIGURE_DISABLE_MERGER:
		case OSYNCTOOL_CONFIGURE_ENABLE_CONVERTER:
		case OSYNCTOOL_CONFIGURE_DISABLE_CONVERTER:
		case OSYNCTOOL_CONFIGURE_FILTER:
		case OSYNCTOOL_SYNC:
			if (argc - optind != 1)
				usage(argv[0], 1);
			groupname = argv[optind];
			break;
		case OSYNCTOOL_CONFIGURE:
		case OSYNCTOOL_CONFIGURE_DELMEMBER:
			if (argc - optind != 2)
				usage(argv[0], 1);
			groupname = argv[optind];
			membername = argv[optind + 1];
		break;
		case OSYNCTOOL_CONFIGURE_ENABLE_OBJTYPE:
		case OSYNCTOOL_CONFIGURE_DISABLE_OBJTYPE:
			if (argc - optind != 2)
				usage (argv[0], 1);
			groupname = argv[optind];
			objtype = argv[optind + 1];
			break;
		case OSYNCTOOL_CONFIGURE_ADDMEMBER:
			if (argc - optind < 2 || argc - optind > 3)
				usage(argv[0], 1);
			groupname = argv[optind];
			pluginname = argv[optind + 1];
			if (argc - optind == 3)
				membername = argv[optind + 2];
			break;
		case OSYNCTOOL_CONFIGURE_MEMBER_ENABLE_WRITE:
		case OSYNCTOOL_CONFIGURE_MEMBER_DISABLE_WRITE:
			if (argc - optind < 2 || argc - optind > 3)
				usage(argv[0], 1);
			groupname = argv[optind];
			membername = argv[optind + 1];
			if (argc - optind == 3)
				objtype = argv[optind + 2];
			break;
		case OSYNCTOOL_CONFIGURE_DISCOVER:
			if (argc - optind < 1 || argc - optind > 2)
				usage(argv[0], 1);
			groupname = argv[optind];
			if (argc - optind == 2)
				membername = argv[optind + 1];
			break;
		default:
			if (argc != optind)
				usage(argv[0], 1);
	}

	if (action != OSYNCTOOL_SYNC) {
		if (slow_objects != NULL ||
		    wait == TRUE ||
		    multi == TRUE ||
		    conflict != OSYNC_CONFLICT_RESOLUTION_UNKNOWN ||
		    manual == TRUE ||
		    always_accept_forecast == TRUE) {
			usage(argv[0], 1);
		}
	}
	
	group_env = osync_group_env_new(&error);
	if (!group_env)
		goto error;
		
	format_env = osync_format_env_new(&error);
	if (!format_env)
		goto error_free_group_env;
		
	plugin_env = osync_plugin_env_new(&error);
	if (!plugin_env)
		goto error_free_format_env;
	
	switch (action) {
		case OSYNCTOOL_SHOW_GROUP:
		case OSYNCTOOL_SHOW_GROUPS:
		case OSYNCTOOL_CONFIGURE_DELMEMBER:
		case OSYNCTOOL_CONFIGURE_ADDGROUP:
		case OSYNCTOOL_CONFIGURE_DELGROUP:
		case OSYNCTOOL_SHOW_OBJTYPES:
		case OSYNCTOOL_SHOW_FILTER:
		case OSYNCTOOL_SHOW_CAPABILITIES:
		case OSYNCTOOL_CONFIGURE_ENABLE_OBJTYPE:
		case OSYNCTOOL_CONFIGURE_DISABLE_OBJTYPE:
		case OSYNCTOOL_CONFIGURE_ENABLE_MERGER:
		case OSYNCTOOL_CONFIGURE_DISABLE_MERGER:
		case OSYNCTOOL_CONFIGURE_ENABLE_CONVERTER:
		case OSYNCTOOL_CONFIGURE_DISABLE_CONVERTER:
		case OSYNCTOOL_CONFIGURE_MEMBER_ENABLE_WRITE:
		case OSYNCTOOL_CONFIGURE_MEMBER_DISABLE_WRITE:
		case OSYNCTOOL_CONFIGURE_FILTER:
			if (!osync_group_env_load_groups(group_env, configdir, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_INSTALL_LISTFORMATS:
			if (!osync_format_env_load_plugins(format_env, NULL, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_INSTALL_LISTPLUGINS:
			if (!osync_plugin_env_load(plugin_env, NULL, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_ADDMEMBER:
		case OSYNCTOOL_SYNC:
		case OSYNCTOOL_CONFIGURE:
		case OSYNCTOOL_CONFIGURE_DISCOVER:
			if (!osync_group_env_load_groups(group_env, configdir, &error))
				goto error_free_plugin_env;
				
			if (!osync_format_env_load_plugins(format_env, NULL, &error))
				goto error_free_plugin_env;
				
			if (!osync_plugin_env_load(plugin_env, NULL, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_INSTALL_GETVERSION:
			break;
		case NONE:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "No action given");
			goto error_free_plugin_env;
	}
	
	switch (action) {
		case OSYNCTOOL_INSTALL_LISTPLUGINS:
			osynctool_list_plugins(plugin_env);
			break;
		case OSYNCTOOL_INSTALL_LISTFORMATS:
			if (!osynctool_list_formats(format_env, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_INSTALL_GETVERSION:
			osynctool_version();
			break;
			
		case OSYNCTOOL_SHOW_GROUPS:
			osynctool_show_groups(group_env);
			break;
		case OSYNCTOOL_SHOW_GROUP:
			if (!osynctool_show_group(group_env, groupname, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_SHOW_OBJTYPES:
			if (!osynctool_show_objtypes(group_env, groupname, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_SHOW_FILTER:
			if (!osynctool_show_filter(group_env, groupname, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_SHOW_CAPABILITIES:
			if (!osynctool_show_capabilities(group_env, groupname, &error))
				goto error_free_plugin_env;	
			break;
		case OSYNCTOOL_CONFIGURE_ADDGROUP:
			if (!osynctool_add_group(group_env, groupname, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_DELGROUP:
			if (!osynctool_del_group(group_env, groupname, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_ADDMEMBER:
			if (!osynctool_add_member(group_env, plugin_env, groupname, pluginname, membername, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE:
			if (!osynctool_configure_member(group_env, plugin_env, groupname, membername, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_DELMEMBER:
			if (!osynctool_del_member(group_env, groupname, membername, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_ENABLE_OBJTYPE:
			if (!osynctool_enable_objtype(group_env, groupname, objtype, TRUE, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_DISABLE_OBJTYPE:
			if (!osynctool_enable_objtype(group_env, groupname, objtype, FALSE, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_ENABLE_MERGER:
			if (!osynctool_configure_group_set_merger_enabled(group_env, groupname, TRUE, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_DISABLE_MERGER:
			if (!osynctool_configure_group_set_merger_enabled(group_env, groupname, FALSE, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_ENABLE_CONVERTER:
			if (!osynctool_configure_group_set_converter_enabled(group_env, groupname, TRUE, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_DISABLE_CONVERTER:
			if (!osynctool_configure_group_set_converter_enabled(group_env, groupname, FALSE, &error))
				goto error_free_plugin_env;
			break;	
		case OSYNCTOOL_CONFIGURE_FILTER:
			if (!osynctool_configure_filter(group_env, groupname, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_DISCOVER:
			if (!osynctool_configure_discover(group_env, plugin_env, groupname, membername, &error))
				goto error_free_plugin_env;
			break;
		case OSYNCTOOL_CONFIGURE_MEMBER_ENABLE_WRITE:
			if  (!osynctool_configure_member_sink(group_env, groupname, membername, objtype, TRUE, &error))
				goto error_free_group_env;
			break;
		case OSYNCTOOL_CONFIGURE_MEMBER_DISABLE_WRITE:
			if  (!osynctool_configure_member_sink(group_env, groupname, membername, objtype, FALSE, &error))
				goto error_free_group_env;
			break;
		case OSYNCTOOL_SYNC:
			if (!osynctool_synchronize(group_env, groupname, wait, multi, slow_objects, &error)) {
				g_list_free(slow_objects);
				goto error_free_plugin_env;
			}
			break;
		case NONE:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "No action given");
			goto error_free_plugin_env;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Internal error, unhandled action selected");
			goto error_free_plugin_env;
	}

	g_list_free(slow_objects);
	osync_group_env_unref(group_env);
	osync_format_env_unref(format_env);
	osync_plugin_env_unref(plugin_env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return 0;

error_free_plugin_env:
	osync_plugin_env_unref(plugin_env);
error_free_format_env:
	osync_format_env_unref(format_env);
error_free_group_env:
	osync_group_env_unref(group_env);
error:
	fprintf(stderr, "ERROR: %s\n", osync_error_print(&error));
	osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	return -1;
}

#endif

