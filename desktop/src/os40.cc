///
/// \file	os40.cc
///		Wrapper class for opensync 0.22 syncing behaviour
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

    Used code from osynctool (GPL v2+) as a guide to the API,
    and copied some of the status messages, as well as one funcion
    directly:
    static const char *OSyncChangeType2String(OSyncChangeType type)
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
#include "osconv40.h"
#include <barry/vsmartptr.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <errno.h>
#include <glib.h>
#include "i18n.h"

// use relative paths to backtrack enough to specify only 0.4x includes
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
	std::string GetErrorMsg();
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
	char*			(*osync_error_print_stack)(OSyncError **error);
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
	osync_memberid		(*osync_member_get_id)(OSyncMember *member);
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
					osync_memberid id);
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
	void			(*osync_plugin_resource_unref)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_config_add_resource)(
					OSyncPluginConfig *config,
					OSyncPluginResource *resource);
	osync_bool		(*osync_plugin_resource_is_enabled)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_enable)(
					OSyncPluginResource *resource,
					osync_bool enable);
	OSyncList*		(*osync_plugin_resource_get_objformat_sinks)(
					OSyncPluginResource *resource);
	const char*		(*osync_objformat_sink_get_objformat)(
					OSyncObjFormatSink *sink);
	const char*		(*osync_objformat_sink_get_config)(
					OSyncObjFormatSink *sink);
	void			(*osync_objformat_sink_set_config)(
					OSyncObjFormatSink *sink,
					const char *config);
	OSyncObjFormatSink*	(*osync_objformat_sink_new)(
					const char *objformat,
					OSyncError **error);
	void			(*osync_plugin_resource_add_objformat_sink)(
					OSyncPluginResource *resource,
					OSyncObjFormatSink *formatsink);
	void			(*osync_objformat_sink_unref)(
					OSyncObjFormatSink *sink);
	const char*		(*osync_plugin_resource_get_preferred_format)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_set_preferred_format)(
					OSyncPluginResource *resource,
					const char *preferred_format);
	const char*		(*osync_plugin_resource_get_mime)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_set_mime)(
					OSyncPluginResource *resource,
					const char *mime);
	const char*		(*osync_plugin_resource_get_objtype)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_set_objtype)(
					OSyncPluginResource *resource,
					const char *objtype);
	const char*		(*osync_plugin_resource_get_path)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_set_path)(
					OSyncPluginResource *resource,
					const char *path);
	const char*		(*osync_plugin_resource_get_url)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_set_url)(
					OSyncPluginResource *resource,
					const char *url);
	const char*		(*osync_plugin_config_get_advancedoption_value_by_name)(
					OSyncPluginConfig *config,
					const char *name);
	OSyncList*		(*osync_plugin_config_get_advancedoptions)(
					OSyncPluginConfig *config);
	void			(*osync_plugin_config_add_advancedoption)(
					OSyncPluginConfig *config,
					OSyncPluginAdvancedOption *option);
	OSyncPluginAdvancedOption* (*osync_plugin_advancedoption_new)(
					OSyncError **error);
	void			(*osync_plugin_advancedoption_unref)(
					OSyncPluginAdvancedOption *option);
	const char*		(*osync_plugin_advancedoption_get_name)(
					OSyncPluginAdvancedOption *option);
	void			(*osync_plugin_advancedoption_set_name)(
					OSyncPluginAdvancedOption *option,
					const char *name);
	void			(*osync_plugin_advancedoption_set_displayname)(
					OSyncPluginAdvancedOption *option,
					const char *displayname);
	void			(*osync_plugin_advancedoption_set_type)(
					OSyncPluginAdvancedOption *option,
					OSyncPluginAdvancedOptionType type);
	void			(*osync_plugin_advancedoption_set_value)(
					OSyncPluginAdvancedOption *option,
					const char *value);
	OSyncList*		(*osync_plugin_config_get_resources)(
					OSyncPluginConfig *config);
	OSyncPluginResource*	(*osync_plugin_resource_ref)(
					OSyncPluginResource *resource);
	OSyncPluginResource*	(*osync_plugin_resource_new)(
					OSyncError **error);
	const char*		(*osync_plugin_resource_get_name)(
					OSyncPluginResource *resource);
	void			(*osync_plugin_resource_set_name)(
					OSyncPluginResource *resource,
					const char *name);
	OSyncPluginAuthentication* (*osync_plugin_config_get_authentication)(
					OSyncPluginConfig *config);
	const char*		(*osync_plugin_authentication_get_password)(
					OSyncPluginAuthentication *auth);
	OSyncPluginAuthentication* (*osync_plugin_authentication_new)(
					OSyncError **error);
	osync_bool		(*osync_plugin_authentication_option_is_supported)(
					OSyncPluginAuthentication *auth,
					OSyncPluginAuthenticationOptionSupportedFlag flag);
	void			(*osync_plugin_authentication_unref)(
					OSyncPluginAuthentication *auth);
	void			(*osync_plugin_config_set_authentication)(
					OSyncPluginConfig *config,
					OSyncPluginAuthentication *auth);
	void			(*osync_plugin_authentication_set_password)(
					OSyncPluginAuthentication *auth,
					const char *password);
	const char*		(*osync_plugin_authentication_get_username)(
					OSyncPluginAuthentication *auth);
	void			(*osync_plugin_authentication_set_username)(
					OSyncPluginAuthentication *auth,
					const char *password);
	void			(*osync_group_set_objtype_enabled)(
					OSyncGroup *group, const char *objtype,
					osync_bool enabled);

	// data pointers
	vLateSmartPtr<OSyncGroupEnv, void(*)(OSyncGroupEnv*)> group_env;
	vLateSmartPtr<OSyncFormatEnv, void(*)(OSyncFormatEnv*)> format_env;
	vLateSmartPtr<OSyncPluginEnv, void(*)(OSyncPluginEnv*)> plugin_env;

	TossError error;
	Converter40 converter;

	OpenSync40Private(OpenSync40 &api)
		: error(this)
		, converter(api)
	{
	}

	// helper functions
	std::string osync_error_print_stack_wrapper(OSyncError **error)
	{
		std::string rmsg;
		char *msg = osync_error_print_stack(error);
		if( msg ) {
			rmsg = msg;
			osync_free(msg);
		}
		else {
			rmsg = _C("(NULL error msg)");
		}
		return rmsg;
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

	virtual bool IsAbortSupported() const;
	virtual bool IsIgnoreSupported() const;
	virtual bool IsKeepNewerSupported() const;

	virtual void Select(int change_id); // takes the id of SyncChange
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

class OS40PluginConfigPrivate
{
public:
	OSyncMember *m_member;
	OSyncPluginConfig *m_config;

	OS40PluginConfigPrivate()
		: m_member(0)
		, m_config(0)
	{
	}
};

class OS40ConfigResourcePrivate
{
public:
	OpenSync40Private *m_privapi;
	OS40PluginConfigPrivate *m_parentpriv;
	OSyncPluginResource *m_resource;

	OS40ConfigResourcePrivate()
		: m_privapi(0)
		, m_parentpriv(0)
		, m_resource(0)
	{
	}
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
		case OSYNC_CHANGE_TYPE_ADDED: return _C("ADDED");
		case OSYNC_CHANGE_TYPE_UNMODIFIED: return _C("UNMODIFIED");
		case OSYNC_CHANGE_TYPE_DELETED: return _C("DELETED");
		case OSYNC_CHANGE_TYPE_MODIFIED: return _C("MODIFIED");
		default:
		case OSYNC_CHANGE_TYPE_UNKNOWN: return "?";
	}
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

bool SyncConflict40Private::IsAbortSupported() const
{
	return true;
}

bool SyncConflict40Private::IsIgnoreSupported() const
{
	return m_priv->osync_mapping_engine_supports_ignore(m_mapping);
}

bool SyncConflict40Private::IsKeepNewerSupported() const
{
	return m_priv->osync_mapping_engine_supports_use_latest(m_mapping);
}

void SyncConflict40Private::Select(int change_id)
{
	OSyncList *c = m_priv->osync_list_nth(m_changes, change_id);
	if( !c )
		throw std::logic_error("Bad change_id");

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
		oss << _C("Problems while aborting: ")
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
			string(_C("Conflict not resolved. ")) + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			_C("Unknown exception caught in conflict_handler()"));
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
		string msg;
		bool error_event = false;

		switch( cb->m_priv->osync_engine_change_update_get_event(status) )
		{
		case OSYNC_ENGINE_CHANGE_EVENT_READ:
			action = _C("Received an entry");
			direction = _C("from");
			msg = OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(change));
			break;

		case OSYNC_ENGINE_CHANGE_EVENT_WRITTEN:
			action = _C("Sent an entry");
			direction = _C("to");
			msg = OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(change));
			break;

		case OSYNC_ENGINE_CHANGE_EVENT_ERROR:
			error_event = true;
			action = _C("Error for entry");
			direction = _C("and");
			msg = cb->m_priv->osync_error_print_stack_wrapper(&(error));
			break;
		}

		if( action ) {
			oss << action << " "
			    << cb->m_priv->osync_change_get_uid(change)
			    << "("
			    << cb->m_priv->osync_objformat_get_name( cb->m_priv->osync_change_get_objformat(change))
			    << ") " << direction << _C(" member ")
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
			string(_C("entry_status error: ")) + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			_C("Unknown exception caught in entry_status()"));
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
			oss << _C("Mapping solved");
			break;

		case OSYNC_ENGINE_MAPPING_EVENT_ERROR:
			error_event = true;
			oss << _C("Mapping error: ")
			    << cb->m_priv->osync_error_print_stack_wrapper(&(error));
			break;
		}

		// call the status handler
		if( oss.str().size() )
			cb->m_status->MappingStatus(oss.str(), error_event);
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string(_C("mapping_status error: ")) + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			_C("Unknown exception caught in mapping_status()"));
	}
}

void engine_status(OSyncEngineUpdate *status, void *cbdata)
{
	CallbackBundle *cb = (CallbackBundle*) cbdata;

	try {
		OSyncError *error = cb->m_priv->osync_engine_update_get_error(status);

		ostringstream oss;
		bool error_event = false;
		bool slow_sync = false;

		switch( cb->m_priv->osync_engine_update_get_event(status) )
		{
		case OSYNC_ENGINE_EVENT_CONNECTED:
			oss << _C("All clients connected or error");
			break;
		case OSYNC_ENGINE_EVENT_CONNECT_DONE:
			/* Not of interest for regular user. */
			break;
		case OSYNC_ENGINE_EVENT_READ:
			oss << _C("All clients sent changes or error");
			break;
		case OSYNC_ENGINE_EVENT_MAPPED:
			oss << _C("All changes got mapped");
			break;
		case OSYNC_ENGINE_EVENT_MULTIPLIED:
			oss << _C("All changes got multiplied");
			break;
		case OSYNC_ENGINE_EVENT_PREPARED_WRITE:
			oss << _C("All changes got prepared for write");
			break;
		case OSYNC_ENGINE_EVENT_PREPARED_MAP:
			/* Not of interest for regular user. */
			break;
		case OSYNC_ENGINE_EVENT_WRITTEN:
			oss << _C("All clients have written");
			break;
		case OSYNC_ENGINE_EVENT_DISCONNECTED:
			oss << _C("All clients have disconnected");
			break;
		case OSYNC_ENGINE_EVENT_ERROR:
			error_event = true;
			oss << _C("The sync failed: ") << cb->m_priv->osync_error_print_stack_wrapper(&(error));
			break;
		case OSYNC_ENGINE_EVENT_SUCCESSFUL:
			oss << _C("The sync was successful");
			break;
		case OSYNC_ENGINE_EVENT_PREV_UNCLEAN:
			oss << _C("The previous synchronization was unclean. Slow-syncing");
			slow_sync = true;
			break;
		case OSYNC_ENGINE_EVENT_END_CONFLICTS:
			oss << _C("All conflicts have been reported");
			break;
		case OSYNC_ENGINE_EVENT_SYNC_DONE:
			oss << _C("All clients reported sync done");
			break;
		}

		// call the status handler
		if( oss.str().size() )
			cb->m_status->EngineStatus(oss.str(),
				error_event,
				slow_sync);
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string(_C("engine_status error: ")) + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			_C("Unknown exception caught in engine_status()"));
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
			oss << _C("Main sink");
		else
			oss << objtype << _C(" sink");


		OSyncMember *member = cb->m_priv->osync_engine_member_update_get_member(status);

		oss << _C(" of member ")
		    << cb->m_priv->osync_member_get_id(member)
		    << " ("
		    << cb->m_priv->osync_member_get_pluginname(member)
		    << ")";

		OSyncError *error = cb->m_priv->osync_engine_member_update_get_error(status);

		switch( cb->m_priv->osync_engine_member_update_get_event(status) )
		{
		case OSYNC_ENGINE_MEMBER_EVENT_CONNECTED:
			oss << _C(" just connected");
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_CONNECT_DONE:
			// Special event - but not interesting for
			// the normal user.
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_DISCONNECTED:
			oss << _C(" just disconnected");
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_READ:
			oss << _C(" just sent all changes");
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_WRITTEN:
			oss << _C(" committed all changes");
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_SYNC_DONE:
			oss << _C(" reported sync done");
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_DISCOVERED:
			oss << _C(" discovered its objtypes");
			break;
		case OSYNC_ENGINE_MEMBER_EVENT_ERROR:
			oss << _C(" had an error: ")
			    << cb->m_priv->osync_error_print_stack_wrapper(&error);
			error_event = true;
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
			string(_C("member_status error: ")) + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			_C("Unknown exception caught in member_status()"));
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
		else {
			// nothing dirty, just continue
			summary.Continue();
		}
	}
	catch( std::exception &e ) {
		cb->m_status->ReportError(
			string(_C("Error handling summary item. ")) + e.what());
	}
	catch( ... ) {
		cb->m_status->ReportError(
			_C("Unknown exception caught in multiply_summary()"));
	}
}


/////////////////////////////////////////////////////////////////////////////
// OS40ConfigResource - public members

OS40ConfigResource::OS40ConfigResource(const OS40PluginConfig &parent,
					void *resource,
					bool existing_resource)
	: m_priv( new OS40ConfigResourcePrivate )
	, m_exists(existing_resource)
{
	m_priv->m_privapi = parent.m_privapi;
	m_priv->m_parentpriv = parent.m_priv.get();
	m_priv->m_resource = (OSyncPluginResource*) resource;
}

OS40ConfigResource::~OS40ConfigResource()
{
	// unref the resource, since we hold a copy
	m_priv->m_privapi->
		osync_plugin_resource_unref(m_priv->m_resource);
	delete m_priv;
}

bool OS40ConfigResource::IsExistingResource() const
{
	return m_exists;
}

// safe to call multiple times
void OS40ConfigResource::AddResource()
{
	if( !IsExistingResource() ) {
		m_priv->m_privapi->
			osync_plugin_config_add_resource(
				m_priv->m_parentpriv->m_config,
				m_priv->m_resource);
	}
}

bool OS40ConfigResource::IsEnabled() const
{
	return m_priv->m_privapi->
		osync_plugin_resource_is_enabled(m_priv->m_resource);
}

OS40ConfigResource& OS40ConfigResource::Enable(bool enabled)
{
	m_priv->m_privapi->osync_plugin_resource_enable(m_priv->m_resource,
		enabled);
	return *this;
}

bool OS40ConfigResource::FindObjFormat(const std::string &objformat,
					std::string &config)
{
	SyncListHandle sinks(m_priv->m_privapi->osync_list_free);
	sinks = m_priv->m_privapi->
		osync_plugin_resource_get_objformat_sinks(m_priv->m_resource);
	for( OSyncList *o = sinks.get(); o; o = o->next ) {
		OSyncObjFormatSink *sink = (OSyncObjFormatSink*) o->data;
		if( objformat == m_priv->m_privapi->osync_objformat_sink_get_objformat(sink) ) {
			const char *cfg = m_priv->m_privapi->osync_objformat_sink_get_config(sink);
			if( cfg )
				config = cfg;
			else
				config.clear();
			return true;
		}
	}
	return false;
}

OS40ConfigResource& OS40ConfigResource::SetObjFormat(const std::string &objformat,
					const std::string &config)
{
	// if it already exists, just set the config value
	SyncListHandle sinks(m_priv->m_privapi->osync_list_free);
	sinks = m_priv->m_privapi->
		osync_plugin_resource_get_objformat_sinks(m_priv->m_resource);
	for( OSyncList *o = sinks.get(); o; o = o->next ) {
		OSyncObjFormatSink *sink = (OSyncObjFormatSink*) o->data;
		if( objformat == m_priv->m_privapi->osync_objformat_sink_get_objformat(sink) ) {
			m_priv->m_privapi->osync_objformat_sink_set_config(sink, config.c_str());
			return *this;
		}
	}

	// if we get here, it doesn't exist, and we need to add it
	OSyncObjFormatSink *sink = m_priv->m_privapi->
		osync_objformat_sink_new(objformat.c_str(),
			m_priv->m_privapi->error);
	if( !sink )
		throw std::runtime_error(m_priv->m_privapi->error.GetErrorMsg());

	if( config.size() )
		m_priv->m_privapi->osync_objformat_sink_set_config(sink,
			config.c_str());
	m_priv->m_privapi->osync_plugin_resource_add_objformat_sink(
		m_priv->m_resource, sink);
	m_priv->m_privapi->osync_objformat_sink_unref(sink);
	return *this;
}

std::string OS40ConfigResource::GetName() const
{
	string value;
	const char *pv = m_priv->m_privapi->
		osync_plugin_resource_get_name(m_priv->m_resource);
	if( pv )
		value = pv;
	return value;
}

OS40ConfigResource& OS40ConfigResource::SetName(const std::string &name)
{
	m_priv->m_privapi->
		osync_plugin_resource_set_name(m_priv->m_resource, name.c_str());
	return *this;
}

std::string OS40ConfigResource::GetPreferredFormat() const
{
	string value;
	const char *pv = m_priv->m_privapi->
		osync_plugin_resource_get_preferred_format(m_priv->m_resource);
	if( pv )
		value = pv;
	return value;
}

OS40ConfigResource& OS40ConfigResource::SetPreferredFormat(const std::string &format)
{
	m_priv->m_privapi->
		osync_plugin_resource_set_preferred_format(m_priv->m_resource,
			format.c_str());
	return *this;
}

std::string OS40ConfigResource::GetMime() const
{
	string value;
	const char *pv = m_priv->m_privapi->osync_plugin_resource_get_mime(
					m_priv->m_resource);
	if( pv )
		value = pv;
	return value;
}

OS40ConfigResource& OS40ConfigResource::SetMime(const std::string &mime)
{
	m_priv->m_privapi->osync_plugin_resource_set_mime(m_priv->m_resource,
		mime.c_str());
	return *this;
}

std::string OS40ConfigResource::GetObjType() const
{
	string value;
	const char *pv = m_priv->m_privapi->osync_plugin_resource_get_objtype(
					m_priv->m_resource);
	if( pv )
		value = pv;
	return value;
}

OS40ConfigResource& OS40ConfigResource::SetObjType(const std::string &objtype)
{
	m_priv->m_privapi->osync_plugin_resource_set_objtype(m_priv->m_resource,
		objtype.c_str());
	return *this;
}

std::string OS40ConfigResource::GetPath() const
{
	string value;
	const char *pv = m_priv->m_privapi->osync_plugin_resource_get_path(
					m_priv->m_resource);
	if( pv )
		value = pv;
	return value;
}

OS40ConfigResource& OS40ConfigResource::SetPath(const std::string &path)
{
	m_priv->m_privapi->osync_plugin_resource_set_path(m_priv->m_resource,
		path.c_str());
	return *this;
}

std::string OS40ConfigResource::GetUrl() const
{
	string value;
	const char *pv = m_priv->m_privapi->osync_plugin_resource_get_url(
					m_priv->m_resource);
	if( pv )
		value = pv;
	return value;
}

OS40ConfigResource& OS40ConfigResource::SetUrl(const std::string &url)
{
	m_priv->m_privapi->osync_plugin_resource_set_url(m_priv->m_resource,
		url.c_str());
	return *this;
}


/////////////////////////////////////////////////////////////////////////////
// OS40PluginConfig - public members

OS40PluginConfig::OS40PluginConfig(OpenSync40Private *privapi,
					void *member,
					void *config)
	: m_privapi(privapi)
{
	m_priv.reset( new OS40PluginConfigPrivate );
	m_priv->m_member = (OSyncMember*) member;
	m_priv->m_config = (OSyncPluginConfig*) config;
}

std::string OS40PluginConfig::GetAdvanced(const std::string &name)
{
	const char *value = m_privapi->osync_plugin_config_get_advancedoption_value_by_name(m_priv->m_config, name.c_str());
	string val;
	if( value )
		val = value;
	return val;
}

void OS40PluginConfig::SetAdvanced(const std::string &name,
				const std::string &display_name,
				const std::string &val)
{
	SetAdvanced(name, display_name, STRING_TYPE, val);
}

void OS40PluginConfig::SetAdvanced(const std::string &name,
				const std::string &display_name,
				int val_type,
				const std::string &val)
{
	// find the first advanced option with this name
	SyncListHandle aos(m_privapi->osync_list_free);
	aos = m_privapi->osync_plugin_config_get_advancedoptions(m_priv->m_config);
	OSyncPluginAdvancedOption *option = 0;
	for( OSyncList *o = aos.get(); o; o = o->next ) {
		option = (OSyncPluginAdvancedOption*) o->data;

		if( name == m_privapi->osync_plugin_advancedoption_get_name(option) )
			break;
	}

	if( option ) {
		// found existing option, set it with val
		m_privapi->osync_plugin_advancedoption_set_value(option, val.c_str());
	}
	else {
		// option with that name does not exist, so create it
		option = m_privapi->osync_plugin_advancedoption_new(m_privapi->error);
		if( !option )
			throw std::runtime_error(m_privapi->error.GetErrorMsg());

		m_privapi->osync_plugin_advancedoption_set_name(option, name.c_str());
		m_privapi->osync_plugin_advancedoption_set_displayname(option, display_name.c_str());
		OSyncPluginAdvancedOptionType type;
		switch( val_type )
		{
		case NONE_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_NONE;
			break;
		case BOOL_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL;
			break;
		case CHAR_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR;
			break;
		case DOUBLE_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_DOUBLE;
			break;
		case INT_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_INT;
			break;
		case LONG_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONG;
			break;
		case LONGLONG_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONGLONG;
			break;
		case UINT_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_UINT;
			break;
		case ULONG_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONG;
			break;
		case ULONGLONG_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONGLONG;
			break;
		case STRING_TYPE:
			type = OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_STRING;
			break;
		default:
			throw std::logic_error("Bad type in SetAdvanced()");
		}
		m_privapi->osync_plugin_advancedoption_set_type(option, type);
		m_privapi->osync_plugin_advancedoption_set_value(option, val.c_str());
		m_privapi->osync_plugin_config_add_advancedoption(m_priv->m_config, option);
		m_privapi->osync_plugin_advancedoption_unref(option);
	}
}

OS40PluginConfig::OS40ConfigResourcePtr
OS40PluginConfig::GetResource(const std::string &objtype)
{
	OS40ConfigResourcePtr ptr;

	// FIXME - get_resources() does not give us a copy, so don't use
	//         the SyncListHandle here
	OSyncList *rs = m_privapi->osync_plugin_config_get_resources(m_priv->m_config);
	for( OSyncList *o = rs; o; o = o->next ) {
		OSyncPluginResource *res = (OSyncPluginResource*) o->data;
		if( objtype == m_privapi->osync_plugin_resource_get_objtype(res) ) {
			// bump the resource count, since OS40ConfigResource
			// will unref it in the destructor
			m_privapi->osync_plugin_resource_ref(res);
			ptr.reset( new OS40ConfigResource(*this, res, true) );
			return ptr;
		}
	}

	// this res has a ref bump already, no ref() needed like it is above
	OSyncPluginResource *res = m_privapi->osync_plugin_resource_new(m_privapi->error);
	if( !res )
		throw std::runtime_error(m_privapi->error.GetErrorMsg());
	ptr.reset( new OS40ConfigResource(*this, res, false) );
	// we search by objtype name, so make sure this is set in
	// the new object
	ptr->SetObjType(objtype);
	return ptr;
}

std::string OS40PluginConfig::GetUsername() const
{
	string username;

	OSyncPluginAuthentication *auth = m_privapi->osync_plugin_config_get_authentication(m_priv->m_config);
	if( !auth )
		return username;

	const char *un = m_privapi->osync_plugin_authentication_get_username(auth);
	if( !un )
		return username;

	username = un;
	return username;
}

void OS40PluginConfig::SetUsername(const std::string &username)
{
	OSyncPluginAuthentication *auth = m_privapi->osync_plugin_config_get_authentication(m_priv->m_config);
	if( !auth ) {
		auth = m_privapi->osync_plugin_authentication_new(m_privapi->error);
		if( !auth )
			throw std::runtime_error(m_privapi->error.GetErrorMsg());
		if( !m_privapi->osync_plugin_authentication_option_is_supported(auth, OSYNC_PLUGIN_AUTHENTICATION_USERNAME) ) {
			m_privapi->osync_plugin_authentication_unref(auth);
			throw std::runtime_error(_C("Username (authentication parameter) is not supported in plugin!"));
		}

		// all looks ok, add it to the config
		m_privapi->osync_plugin_config_set_authentication(m_priv->m_config, auth);
		// unref our copy, since the config now has it...
		// our auth pointer will still be valid since config holds it
		m_privapi->osync_plugin_authentication_unref(auth);
	}

	m_privapi->osync_plugin_authentication_set_username(auth, username.c_str());
}

std::string OS40PluginConfig::GetPassword() const
{
	string password;

	OSyncPluginAuthentication *auth = m_privapi->osync_plugin_config_get_authentication(m_priv->m_config);
	if( !auth )
		return password;

	const char *pass = m_privapi->osync_plugin_authentication_get_password(auth);
	if( !pass )
		return password;

	password = pass;
	return password;
}

void OS40PluginConfig::SetPassword(const std::string &password)
{
	OSyncPluginAuthentication *auth = m_privapi->osync_plugin_config_get_authentication(m_priv->m_config);
	if( !auth ) {
		auth = m_privapi->osync_plugin_authentication_new(m_privapi->error);
		if( !auth )
			throw std::runtime_error(m_privapi->error.GetErrorMsg());
		if( !m_privapi->osync_plugin_authentication_option_is_supported(auth, OSYNC_PLUGIN_AUTHENTICATION_PASSWORD) ) {
			m_privapi->osync_plugin_authentication_unref(auth);
			throw std::runtime_error(_C("Password authentication is not supported in plugin!"));
		}

		// all looks ok, add it to the config
		m_privapi->osync_plugin_config_set_authentication(m_priv->m_config, auth);
		// unref our copy, since the config now has it...
		// our auth pointer will still be valid since config holds it
		m_privapi->osync_plugin_authentication_unref(auth);
	}

	m_privapi->osync_plugin_authentication_set_password(auth, password.c_str());
}

void OS40PluginConfig::Save()
{
	if( !m_privapi->osync_member_save(m_priv->m_member, m_privapi->error) )
		throw std::runtime_error(m_privapi->error.GetErrorMsg());
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
	std::auto_ptr<OpenSync40Private> p(new OpenSync40Private(*this));

	// load all required symbols...
	// we don't need to use try/catch here, since the base
	// class destructor will clean up for us if LoadSym() throws
	LoadSym(p->osync_get_version, "osync_get_version");
	LoadSym(p->osync_error_print, "osync_error_print");
	LoadSym(p->osync_error_print_stack, "osync_error_print_stack");
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
	LoadSym(p->osync_plugin_resource_unref,
				"osync_plugin_resource_unref");
	LoadSym(p->osync_plugin_config_add_resource,
				"osync_plugin_config_add_resource");
	LoadSym(p->osync_plugin_resource_is_enabled,
				"osync_plugin_resource_is_enabled");
	LoadSym(p->osync_plugin_resource_enable,
				"osync_plugin_resource_enable");
	LoadSym(p->osync_plugin_resource_get_objformat_sinks,
				"osync_plugin_resource_get_objformat_sinks");
	LoadSym(p->osync_objformat_sink_get_objformat,
				"osync_objformat_sink_get_objformat");
	LoadSym(p->osync_objformat_sink_get_config,
				"osync_objformat_sink_get_config");
	LoadSym(p->osync_objformat_sink_set_config,
				"osync_objformat_sink_set_config");
	LoadSym(p->osync_objformat_sink_new,
				"osync_objformat_sink_new");
	LoadSym(p->osync_plugin_resource_add_objformat_sink,
				"osync_plugin_resource_add_objformat_sink");
	LoadSym(p->osync_objformat_sink_unref,
				"osync_objformat_sink_unref");
	LoadSym(p->osync_plugin_resource_get_preferred_format,
				"osync_plugin_resource_get_preferred_format");
	LoadSym(p->osync_plugin_resource_set_preferred_format,
				"osync_plugin_resource_set_preferred_format");
	LoadSym(p->osync_plugin_resource_get_mime,
				"osync_plugin_resource_get_mime");
	LoadSym(p->osync_plugin_resource_set_mime,
				"osync_plugin_resource_set_mime");
	LoadSym(p->osync_plugin_resource_get_objtype,
				"osync_plugin_resource_get_objtype");
	LoadSym(p->osync_plugin_resource_set_objtype,
				"osync_plugin_resource_set_objtype");
	LoadSym(p->osync_plugin_resource_get_path,
				"osync_plugin_resource_get_path");
	LoadSym(p->osync_plugin_resource_set_path,
				"osync_plugin_resource_set_path");
	LoadSym(p->osync_plugin_resource_get_url,
				"osync_plugin_resource_get_url");
	LoadSym(p->osync_plugin_resource_set_url,
				"osync_plugin_resource_set_url");
	LoadSym(p->osync_plugin_config_get_advancedoption_value_by_name,
			"osync_plugin_config_get_advancedoption_value_by_name");
	LoadSym(p->osync_plugin_config_get_advancedoptions,
				"osync_plugin_config_get_advancedoptions");
	LoadSym(p->osync_plugin_config_add_advancedoption,
				"osync_plugin_config_add_advancedoption");
	LoadSym(p->osync_plugin_advancedoption_new,
				"osync_plugin_advancedoption_new");
	LoadSym(p->osync_plugin_advancedoption_unref,
				"osync_plugin_advancedoption_unref");
	LoadSym(p->osync_plugin_advancedoption_get_name,
				"osync_plugin_advancedoption_get_name");
	LoadSym(p->osync_plugin_advancedoption_set_name,
				"osync_plugin_advancedoption_set_name");
	LoadSym(p->osync_plugin_advancedoption_set_displayname,
				"osync_plugin_advancedoption_set_displayname");
	LoadSym(p->osync_plugin_advancedoption_set_type,
				"osync_plugin_advancedoption_set_type");
	LoadSym(p->osync_plugin_advancedoption_set_value,
				"osync_plugin_advancedoption_set_value");
	LoadSym(p->osync_plugin_config_get_resources,
				"osync_plugin_config_get_resources");
	LoadSym(p->osync_plugin_resource_ref,
				"osync_plugin_resource_ref");
	LoadSym(p->osync_plugin_resource_new,
				"osync_plugin_resource_new");
	LoadSym(p->osync_plugin_resource_get_name,
				"osync_plugin_resource_get_name");
	LoadSym(p->osync_plugin_resource_set_name,
				"osync_plugin_resource_set_name");
	LoadSym(p->osync_plugin_config_get_authentication,
				"osync_plugin_config_get_authentication");
	LoadSym(p->osync_plugin_authentication_get_password,
				"osync_plugin_authentication_get_password");
	LoadSym(p->osync_plugin_authentication_new,
				"osync_plugin_authentication_new");
	LoadSym(p->osync_plugin_authentication_option_is_supported,
			"osync_plugin_authentication_option_is_supported");
	LoadSym(p->osync_plugin_authentication_unref,
				"osync_plugin_authentication_unref");
	LoadSym(p->osync_plugin_config_set_authentication,
				"osync_plugin_config_set_authentication");
	LoadSym(p->osync_plugin_authentication_set_password,
				"osync_plugin_authentication_set_password");
	LoadSym(p->osync_plugin_authentication_get_username,
				"osync_plugin_authentication_get_username");
	LoadSym(p->osync_plugin_authentication_set_username,
				"osync_plugin_authentication_set_username");
	LoadSym(p->osync_group_set_objtype_enabled,
				"osync_group_set_objtype_enabled");

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

const char* OpenSync40::GetEngineName() const
{
	return "0.40";
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
		oss << "GetMembers(): " << _C("Unable to find group with name: ") << group_name;
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

		new_member.group_name = group_name;
		new_member.id = m_priv->osync_member_get_id(member);
		new_member.plugin_name = m_priv->osync_member_get_pluginname(member);

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
		throw std::runtime_error("AddGroup(): " + m_priv->error.GetErrorMsg());

	m_priv->osync_group_set_name(group, group_name.c_str());
	if( !m_priv->osync_group_env_add_group(m_priv->group_env.get(), group, m_priv->error) ) {
		m_priv->osync_group_unref(group);
		throw std::runtime_error("AddGroup(): " + m_priv->error.GetErrorMsg());
	}

	if( !m_priv->osync_group_save(group, m_priv->error) ) {
		m_priv->osync_group_unref(group);
		throw std::runtime_error("AddGroup(): " + m_priv->error.GetErrorMsg());
	}

	m_priv->osync_group_unref(group);
}

void OpenSync40::DeleteGroup(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("DeleteGroup(): ") + _C("Group not found: ") + group_name);

	if( !m_priv->osync_group_delete(group, m_priv->error) )
		throw std::runtime_error("DeleteGroup(): " + m_priv->error.GetErrorMsg());

	m_priv->osync_group_env_remove_group(m_priv->group_env.get(), group);
}

Converter& OpenSync40::GetConverter()
{
	return m_priv->converter;
}

long OpenSync40::AddMember(const std::string &group_name,
			const std::string &plugin_name,
			const std::string &member_name)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("AddMember(): ") + _C("Group not found: ") + group_name);

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), plugin_name.c_str());
	if( !plugin )
		throw std::runtime_error(string("AddMember(): ") + _C("Plugin not found: ") + plugin_name);

	vLateSmartPtr<OSyncMember, void(*)(OSyncMember*)> mptr(m_priv->osync_member_unref);
	mptr = m_priv->osync_member_new(m_priv->error);
	if( !mptr.get() )
		throw std::runtime_error("AddMember(): " + m_priv->error.GetErrorMsg());

	m_priv->osync_group_add_member(group, mptr.get());
	m_priv->osync_member_set_pluginname(mptr.get(), plugin_name.c_str());

	if( member_name.size() )
		m_priv->osync_member_set_name(mptr.get(), member_name.c_str());

	if( !m_priv->osync_member_save(mptr.get(), m_priv->error) )
		throw std::runtime_error("AddMember(): " + m_priv->error.GetErrorMsg());

	return m_priv->osync_member_get_id(mptr.get());
}

void OpenSync40::DeleteMember(const std::string &group_name, long member_id)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("DeleteMember(): ") + _C("Group not found: ") + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "DeleteMember(): " << _C("Member not found: ") << member_id;
		throw std::runtime_error(oss.str());
	}

	if( !m_priv->osync_member_delete(member, m_priv->error) )
		throw std::runtime_error("DeleteMember(): " + m_priv->error.GetErrorMsg());

	m_priv->osync_group_remove_member(group, member);
}

void OpenSync40::DeleteMember(const std::string &group_name,
				const std::string &plugin_name)
{
	member_list_type mlist;
	GetMembers(group_name, mlist);
	Member *member = mlist.Find(plugin_name.c_str());
	if( !member )
		throw std::runtime_error(string("DeleteMember(): ") + _C("Member not found: ") + plugin_name);

	DeleteMember(group_name, member->id);
}

bool OpenSync40::IsConfigurable(const std::string &group_name,
				long member_id)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("IsConfigurable(): ") + _C("Group not found: ") + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "IsConfigurable(): " << _C("Member not found: ") << member_id;
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("IsConfigurable(): ") + _C("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


	OSyncPluginConfigurationType type = m_priv->osync_plugin_get_config_type(plugin);
	return type != OSYNC_PLUGIN_NO_CONFIGURATION;
}

std::string OpenSync40::GetConfiguration(const std::string &group_name,
					long member_id)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "GetConfiguration(): " << _C("Member ") << member_id << _C(" of group '") << group_name << _C("' does not accept configuration.");
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("GetConfiguration(): ") + _C("Group not found: ") + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "GetConfiguration(): " << _C("Member not found: ") << member_id;
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("GetConfiguration(): ") + _C("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


	OSyncPluginConfig *config = m_priv->osync_member_get_config_or_default(member, m_priv->error);
	if( !config )
		throw std::runtime_error("GetConfiguration(): " + m_priv->error.GetErrorMsg());

	// To emulate 0.22 behaviour, we need to use 0.4x save-to-file
	// functions, and then load that from the file again, and
	// return that string as the configuratin.

	TempDir tempdir("opensyncapi");

	string filename = tempdir.GetNewFilename();

	if( !m_priv->osync_plugin_config_file_save(config, filename.c_str(), m_priv->error) )
		throw std::runtime_error("GetConfiguration(): " + m_priv->error.GetErrorMsg());

	ifstream in(filename.c_str());
	string config_data;
	char buf[4096];
	while( in ) {
		in.read(buf, sizeof(buf));
		config_data.append(buf, in.gcount());
	}

	return config_data;
}

OS40PluginConfig OpenSync40::GetConfigurationObj(const std::string &group_name,
						long member_id)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "GetConfigurationObj(): " << _C("Member ") << member_id << _C(" of group '") << group_name << _C("' does not accept configuration.");
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("GetConfigurationObj(): ") + _C("Group not found: ") + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "GetConfigurationObj(): " << _C("Member not found: ") << member_id;
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("GetConfigurationObj(): ") + _C("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


	OSyncPluginConfig *config = m_priv->osync_member_get_config_or_default(member, m_priv->error);
	if( !config )
		throw std::runtime_error("GetConfigurationObj(): " + m_priv->error.GetErrorMsg());

	return OS40PluginConfig(m_priv, member, config);
}

void OpenSync40::SetConfiguration(const std::string &group_name,
				long member_id,
				const std::string &config_data)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "SetConfiguration(): " << _C("Member ") << member_id << _C(" of group '") << group_name << _C("' does not accept configuration.");
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("SetConfiguration(): ") + _C("Group not found: ") + group_name);

	OSyncMember *member = m_priv->osync_group_find_member(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "SetConfiguration(): " << _C("Member not found: ") << member_id;
		throw std::runtime_error(oss.str());
	}

	OSyncPlugin *plugin = m_priv->osync_plugin_env_find_plugin(m_priv->plugin_env.get(), m_priv->osync_member_get_pluginname(member));
	if( !plugin )
		throw std::runtime_error(string("SetConfiguration(): ") + _C("Unable to find plugin with name: ") + m_priv->osync_member_get_pluginname(member));


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
		throw std::runtime_error("SetConfiguration(): " + m_priv->error.GetErrorMsg());

	m_priv->osync_member_set_config(member, new_config);
	
	if( !m_priv->osync_member_save(member, m_priv->error))
		throw std::runtime_error("SetConfiguration(): " + m_priv->error.GetErrorMsg());
}

void OpenSync40::Discover(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("Discover(): ") + _C("Group not found: ") + group_name);

	EngineHandle engine(m_priv->osync_engine_unref);
	engine = m_priv->osync_engine_new(group, m_priv->error);
	if( !engine.get() )
		throw std::runtime_error("Discover(): " + m_priv->error.GetErrorMsg());

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
			m_priv->osync_error_set(m_priv->error, OSYNC_ERROR_GENERIC, _C("discover failed: no objtypes returned"));
			break;
		}

		if( !m_priv->osync_member_save(member, m_priv->error) )
			break;
	}

	// check for error
	if( m ) {
		m_priv->osync_engine_finalize(engine.get(), m_priv->error);
		throw std::runtime_error("Discover(): " + m_priv->error.GetErrorMsg());
	}
}

void OpenSync40::Sync(const std::string &group_name,
			SyncStatus &status_callback,
			Config::pst_type sync_types)
{
	OSyncGroup *group = m_priv->osync_group_env_find_group(m_priv->group_env.get(), group_name.c_str());
	if( !group )
		throw std::runtime_error(string("Sync(): ") + _C("Group not found: ") + group_name);

	// enable/disable each objtype, as per sync_types
	if( !(sync_types & PST_DO_NOT_SET) ) {
		cerr << "enabling objtypes: " << sync_types << endl;
		m_priv->osync_group_set_objtype_enabled(group, "contact",
			(sync_types & PST_CONTACTS) ? TRUE : FALSE);
		m_priv->osync_group_set_objtype_enabled(group, "event",
			(sync_types & PST_EVENTS) ? TRUE : FALSE);
		m_priv->osync_group_set_objtype_enabled(group, "note",
			(sync_types & PST_NOTES) ? TRUE : FALSE);
		m_priv->osync_group_set_objtype_enabled(group, "todo",
			(sync_types & PST_TODOS) ? TRUE : FALSE);
	}

	EngineHandle engine(m_priv->osync_engine_unref);
	engine = m_priv->osync_engine_new(group, m_priv->error);
	if( !engine.get() )
		throw std::runtime_error("Sync(): " + m_priv->error.GetErrorMsg());


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
			cout << "Sync(): " << _C("Member ") << m_priv->osync_member_get_id(member) << _C(" has no objtypes. Has it already been discovered?") << endl;
		}
	}

	if( !m_priv->osync_engine_initialize(engine.get(), m_priv->error) )
		throw std::runtime_error("Sync(): " + m_priv->error.GetErrorMsg());

	if( !m_priv->osync_engine_synchronize_and_block(engine.get(), m_priv->error) ) {
		m_priv->osync_engine_finalize(engine.get(), NULL);
		throw std::runtime_error("Sync(): " + m_priv->error.GetErrorMsg());
	}

	if( !m_priv->osync_engine_finalize(engine.get(), m_priv->error) )
		throw std::runtime_error("Sync(): " + m_priv->error.GetErrorMsg());
}


/////////////////////////////////////////////////////////////////////////////
// TossError public members

/// Returns NULL if no error
std::string TossError::GetErrorMsg()
{
	return std::string(m_priv->osync_error_print_stack_wrapper(&m_error));
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

