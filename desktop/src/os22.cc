///
/// \file	os22.cc
///		Wrapper class for opensync 0.22 syncing behaviour
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

    Used code from msynctool (GPL v2+) as a guide to the API,
    and copied some of its status messages and one function directly:
    static const char *OSyncChangeType2String(OSyncChangeType c);
    Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>

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

#include "os22.h"
#include "osprivatebase.h"
#include "osconv22.h"
#include <barry/barry.h>
#include <barry/vsmartptr.h>
#include <memory>
#include <glib.h>

#include <../opensync-1.0/opensync/opensync.h>
#include <../opensync-1.0/osengine/engine.h>

using namespace std;

namespace OpenSync {

typedef Barry::vLateSmartPtr<OSyncEngine, void(*)(OSyncEngine*)> EngineHandle;

class OpenSync22Private
{
public:
	// function pointers
	const char*		(*osync_get_version)();
	OSyncEnv*		(*osync_env_new)();
	void			(*osync_env_free)(OSyncEnv *env);
	void			(*osync_env_set_option)(OSyncEnv *env,
					const char *name, const char *value);
	osync_bool		(*osync_env_finalize)(OSyncEnv *env,
					OSyncError **error);
	int			(*osync_env_num_plugins)(OSyncEnv *env);
	OSyncPlugin*		(*osync_env_nth_plugin)(OSyncEnv *env, int nth);
	const char*		(*osync_plugin_get_name)(OSyncPlugin *plugin);
	void			(*osync_error_free)(OSyncError **error);
	const char*		(*osync_error_print)(OSyncError **error);
	osync_bool		(*osync_env_initialize)(OSyncEnv *env,
					OSyncError **error);
	int			(*osync_env_num_groups)(OSyncEnv *env);
	OSyncGroup*		(*osync_env_nth_group)(OSyncEnv *env, int nth);
	const char*		(*osync_group_get_name)(OSyncGroup *group);
	OSyncGroup*		(*osync_env_find_group)(OSyncEnv *env,
					const char *name);
	int			(*osync_group_num_members)(OSyncGroup *group);
	OSyncMember*		(*osync_group_nth_member)(OSyncGroup *group,
					int nth);
	long long int		(*osync_member_get_id)(OSyncMember *member);
	const char*		(*osync_member_get_pluginname)(
					OSyncMember *member);
	OSyncFormatEnv*		(*osync_conv_env_new)(OSyncEnv *env);
	void			(*osync_conv_env_free)(OSyncFormatEnv *env);
	int			(*osync_conv_num_objtypes)(OSyncFormatEnv *env);
	OSyncObjType*		(*osync_conv_nth_objtype)(OSyncFormatEnv *env,
					int nth);
	int			(*osync_conv_num_objformats)(
					OSyncObjType *type);
	OSyncObjFormat*		(*osync_conv_nth_objformat)(OSyncObjType *type,
					int nth);
	const char*		(*osync_objformat_get_name)(
					OSyncObjFormat *format);
	const char*		(*osync_objtype_get_name)(OSyncObjType *type);
	OSyncGroup*		(*osync_group_new)(OSyncEnv *env);
	void			(*osync_group_set_name)(OSyncGroup *group,
					const char *name);
	osync_bool		(*osync_group_save)(OSyncGroup *group,
					OSyncError **error);
	osync_bool		(*osync_group_delete)(OSyncGroup *group,
					OSyncError **error);
	OSyncMember*		(*osync_member_new)(OSyncGroup *group);
	osync_bool		(*osync_member_instance_plugin)(
					OSyncMember *member,
					const char *pluginname,
					OSyncError **error);
	osync_bool		(*osync_member_save)(OSyncMember *member,
					OSyncError **error);
	OSyncMember*		(*osync_member_from_id)(OSyncGroup *group,
					int id);
	osync_bool		(*osync_member_need_config)(OSyncMember *member,
					OSyncConfigurationTypes *type,
					OSyncError **error);
	osync_bool		(*osync_member_get_config_or_default)(
					OSyncMember *member,
					char **data, int *size,
					OSyncError **error);
	void			(*osync_member_set_config)(OSyncMember *member,
					const char *data, int size);
	osync_bool		(*osengine_mapping_ignore_supported)(
					OSyncEngine *engine,
					OSyncMapping *mapping);
	osync_bool		(*osengine_mapping_check_timestamps)(
					OSyncEngine *engine,
					OSyncMapping *mapping,
					OSyncError **error);
	OSyncChange*		(*osengine_mapping_nth_change)(
					OSyncMapping *mapping, int nth);
	void			(*osengine_mapping_solve)(OSyncEngine *engine,
					OSyncMapping *mapping,
					OSyncChange *change);
	void			(*osengine_mapping_duplicate)(
					OSyncEngine *engine,
					OSyncMapping *dupe_mapping);
	osync_bool		(*osengine_mapping_ignore_conflict)(
					OSyncEngine *engine,
					OSyncMapping *mapping,
					OSyncError **error);
	osync_bool		(*osengine_mapping_solve_latest)(
					OSyncEngine *engine,
					OSyncMapping *mapping,
					OSyncError **error);
	const char*		(*osync_change_get_uid)(OSyncChange *change);
	osync_bool		(*osengine_init)(OSyncEngine *engine,
					OSyncError **error);
	OSyncMember*		(*osync_change_get_member)(OSyncChange *change);
	int			(*osync_change_get_datasize)(
					OSyncChange *change);
	OSyncEngine*		(*osengine_new)(OSyncGroup *group,
					OSyncError **error);
	void			(*osengine_free)(OSyncEngine *engine);
	void			(*osengine_finalize)(OSyncEngine *engine);
	osync_bool		(*osengine_sync_and_block)(OSyncEngine *engine,
					OSyncError **error);
	void			(*osengine_set_memberstatus_callback)(
					OSyncEngine *engine,
					void (* function) (OSyncMemberUpdate *,
							void *),
					void *user_data);
	void			(*osengine_set_changestatus_callback)(
					OSyncEngine *engine,
					void (* function) (OSyncEngine *,
							OSyncChangeUpdate *,
							void *),
					void *user_data);
	void			(*osengine_set_enginestatus_callback)(
					OSyncEngine *engine,
					void (* function) (OSyncEngine *,
							OSyncEngineUpdate *,
							void *),
					void *user_data);
	void			(*osengine_set_mappingstatus_callback)(
					OSyncEngine *engine,
					void (* function) (OSyncMappingUpdate *,
							void *),
					void *user_data);
	void			(*osengine_set_conflict_callback)(
					OSyncEngine *engine,
					void (* function) (OSyncEngine *,
							OSyncMapping *,
							void *),
					void *user_data);
	int			(*osengine_mapping_num_changes)(
					OSyncMapping *mapping);
	OSyncChangeType		(*osync_change_get_changetype)(
					OSyncChange *change);
	char*			(*osync_change_get_printable)(
					OSyncChange *change);

	// data pointers
	OSyncEnv *env;

	Converter22 converter;

	OpenSync22Private(OpenSync22 &api)
		: env(0)
		, converter(api)
	{
	}
};

class SyncConflict22Private : public SyncConflictPrivateBase
{
	OpenSync22Private *m_priv;
	OSyncEngine *m_engine;
	OSyncMapping *m_mapping;

public:
	SyncConflict22Private(OpenSync22Private *priv,
		OSyncEngine *engine, OSyncMapping *mapping);
	~SyncConflict22Private();

	virtual bool IsAbortSupported() const;
	virtual bool IsIgnoreSupported() const;
	virtual bool IsKeepNewerSupported() const;

	virtual void Select(int change_id); // takes id of SyncChange object
	virtual void Abort();	// not supported in 0.22
	virtual void Duplicate();
	virtual void Ignore();
	virtual void KeepNewer();

	void AppendChanges(std::vector<SyncChange> &list);
};

struct CallbackBundle22
{
	OpenSync22Private *m_priv;
	SyncStatus *m_status;

	CallbackBundle22(OpenSync22Private *priv, SyncStatus &status)
		: m_priv(priv)
		, m_status(&status)
	{
	}
};

void member_status(OSyncMemberUpdate *, void *);
void entry_status(OSyncEngine *, OSyncChangeUpdate *, void *);
void engine_status(OSyncEngine *, OSyncEngineUpdate *,void *);
void mapping_status(OSyncMappingUpdate *, void *);
void conflict_handler(OSyncEngine *, OSyncMapping *, void *);


/////////////////////////////////////////////////////////////////////////////
// Static helper functions

static const char *OSyncChangeType2String(OSyncChangeType c)
{
	switch (c) {
		case CHANGE_ADDED: return "ADDED";
		case CHANGE_UNMODIFIED: return "UNMODIFIED";
		case CHANGE_DELETED: return "DELETED";
		case CHANGE_MODIFIED: return "MODIFIED";
		default:
		case CHANGE_UNKNOWN: return "?";
	}
}

/////////////////////////////////////////////////////////////////////////////
// SyncConflict22Private member functions

SyncConflict22Private::SyncConflict22Private(OpenSync22Private *priv,
			OSyncEngine *engine, OSyncMapping *mapping)
	: m_priv(priv)
	, m_engine(engine)
	, m_mapping(mapping)
{
}

SyncConflict22Private::~SyncConflict22Private()
{
}

bool SyncConflict22Private::IsAbortSupported() const
{
	return false;	// Abort not explicitly supported in 0.22
}

bool SyncConflict22Private::IsIgnoreSupported() const
{
	return m_priv->osengine_mapping_ignore_supported(m_engine, m_mapping);
}

bool SyncConflict22Private::IsKeepNewerSupported() const
{
	return m_priv->osengine_mapping_check_timestamps(m_engine, m_mapping, NULL); 
}

void SyncConflict22Private::Select(int change_id)
{
	OSyncChange *change = m_priv->osengine_mapping_nth_change(m_mapping, change_id);
	if( !change ) {
		throw std::runtime_error("Bad change_id, or error getting nth change object.");
	}

	m_priv->osengine_mapping_solve(m_engine, m_mapping, change);
}

void SyncConflict22Private::Abort()
{
	throw std::logic_error("Conflict::Abort() not supported in 0.22");
}

void SyncConflict22Private::Duplicate()
{
	m_priv->osengine_mapping_duplicate(m_engine, m_mapping);
}

void SyncConflict22Private::Ignore()
{
	if( !IsIgnoreSupported() )
		throw std::logic_error("Ignore not supported, yet Ignore() called.");

	OSyncError *error = NULL;
	if( !m_priv->osengine_mapping_ignore_conflict(m_engine, m_mapping, &error)) {
		ostringstream oss;
		oss << "Conflict not ignored: "
		    << m_priv->osync_error_print(&error);
		m_priv->osync_error_free(&error);
		throw std::runtime_error(oss.str());
	}
}

void SyncConflict22Private::KeepNewer()
{
	if( !IsKeepNewerSupported() )
		throw std::logic_error("Keep Newer not supported, yet KeepNewer() called.");

	OSyncError *error = NULL;
	if( !m_priv->osengine_mapping_solve_latest(m_engine, m_mapping, &error)) {
		ostringstream oss;
		oss << "Conflict not resolved: "
		    << m_priv->osync_error_print(&error);
		m_priv->osync_error_free(&error);
		throw std::runtime_error(oss.str());
	}
}

void SyncConflict22Private::AppendChanges(std::vector<SyncChange> &list)
{
	for( int i = 0; i < m_priv->osengine_mapping_num_changes(m_mapping); i++ ) {
		OSyncChange *change = m_priv->osengine_mapping_nth_change(m_mapping, i);
		if( m_priv->osync_change_get_changetype(change) != CHANGE_UNKNOWN ) {

			SyncChange entry;

			char *printable = m_priv->osync_change_get_printable(change);
			if( printable ) {
				entry.printable_data = printable;
				g_free(printable);
			}

			OSyncMember *member = m_priv->osync_change_get_member(change);

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
// Callback functions

void member_status(OSyncMemberUpdate *status, void *cbdata)
{
	CallbackBundle22 *cb = (CallbackBundle22*) cbdata;

	try {
		ostringstream oss;
		bool error_event = false;
		bool valid = true;

		oss << "Member "
		    << cb->m_priv->osync_member_get_id(status->member)
		    << " ("
		    << cb->m_priv->osync_member_get_pluginname(status->member)
		    << ")";

		switch( status->type )
		{
		case MEMBER_CONNECTED:
			oss << " just connected";
			break;
		case MEMBER_DISCONNECTED:
			oss << " just disconnected";
			break;
		case MEMBER_SENT_CHANGES:
			oss << " just sent all changes";
			break;
		case MEMBER_COMMITTED_ALL:
			oss << " committed all changes";
			break;
		case MEMBER_CONNECT_ERROR:
			oss << " had an error while connecting: "
			    << cb->m_priv->osync_error_print(&status->error);
			break;
		case MEMBER_GET_CHANGES_ERROR:
			oss << " had an error while getting changes: "
			    << cb->m_priv->osync_error_print(&status->error);
			error_event = true;
			break;
		case MEMBER_SYNC_DONE_ERROR:
			oss << " had an error while calling sync done: "
			    << cb->m_priv->osync_error_print(&status->error);
			error_event = true;
			break;
		case MEMBER_DISCONNECT_ERROR:
			oss << " had an error while disconnecting: "
			    << cb->m_priv->osync_error_print(&status->error);
			error_event = true;
			break;
		case MEMBER_COMMITTED_ALL_ERROR:
			oss << " had an error while commiting changes: "
			    << cb->m_priv->osync_error_print(&status->error);
			error_event = true;
			break;
		default:
			valid = false;
			break;
		}

		// call the status handler
		if( oss.str().size() && valid ) {
			cb->m_status->MemberStatus(
				cb->m_priv->osync_member_get_id(status->member),
				cb->m_priv->osync_member_get_pluginname(status->member),
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

void entry_status(OSyncEngine *engine, OSyncChangeUpdate *status, void *cbdata)
{
	CallbackBundle22 *cb = (CallbackBundle22*) cbdata;

	try {
		ostringstream oss;

		OSyncMember *member = cb->m_priv->osync_change_get_member(status->change);
		bool error_event = false;

		switch( status->type )
		{
		case CHANGE_RECEIVED_INFO:
			oss << "Received an entry "
			    << cb->m_priv->osync_change_get_uid(status->change)
			    << " without data from member "
			    << status->member_id
			    << " ("
			    << cb->m_priv->osync_member_get_pluginname(member)
			    << "). "
			    << "Changetype "
			    << OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(status->change));
			break;
		case CHANGE_RECEIVED:
			oss << "Received an entry "
			    << cb->m_priv->osync_change_get_uid(status->change)
			    << " with data of size "
			    << cb->m_priv->osync_change_get_datasize(status->change)
			    << " from member "
			    << status->member_id
			    << " ("
			    << cb->m_priv->osync_member_get_pluginname(member)
			    << "). Changetype "
			    << OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(status->change));
			break;
		case CHANGE_SENT:
			oss << "Sent an entry "
			    << cb->m_priv->osync_change_get_uid(status->change)
			    << " of size "
			    << cb->m_priv->osync_change_get_datasize(status->change)
			    << " to member "
			    << status->member_id
			    << " ("
			    << cb->m_priv->osync_member_get_pluginname(member)
			    << "). Changetype "
			    << OSyncChangeType2String(cb->m_priv->osync_change_get_changetype(status->change));
			break;
		case CHANGE_WRITE_ERROR:
			error_event = true;
			oss << "Error writing entry "
			    << cb->m_priv->osync_change_get_uid(status->change)
			    << " to member "
			    << status->member_id
			    << " ("
			    << cb->m_priv->osync_member_get_pluginname(member)
			    << "): "
			    << cb->m_priv->osync_error_print(&status->error);
			break;
		case CHANGE_RECV_ERROR:
			error_event = true;
			oss << "Error reading entry "
			    << cb->m_priv->osync_change_get_uid(status->change)
			    << " from member "
			    << status->member_id
			    << " ("
			    << cb->m_priv->osync_member_get_pluginname(member)
			    << "): "
			    << cb->m_priv->osync_error_print(&(status->error));
			break;
		}

		// call the status handler
		if( oss.str().size() ) {
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

void engine_status(OSyncEngine *engine, OSyncEngineUpdate *status, void *cbdata)
{
	CallbackBundle22 *cb = (CallbackBundle22*) cbdata;

	try {
		ostringstream oss;
		bool error_event = false;

		switch( status->type )
		{
		case ENG_PREV_UNCLEAN:
			oss << "The previous synchronization was unclean. Slow-syncing.";
			break;
		case ENG_ENDPHASE_CON:
			oss << "All clients connected or error";
			break;
		case ENG_END_CONFLICTS:
			oss << "All conflicts have been reported";
			break;
		case ENG_ENDPHASE_READ:
			oss << "All clients sent changes or error";
			break;
		case ENG_ENDPHASE_WRITE:
			oss << "All clients have written";
			break;
		case ENG_ENDPHASE_DISCON:
			oss << "All clients have disconnected";
			break;
		case ENG_SYNC_SUCCESSFULL:
			oss << "The sync was successful";
			break;
		case ENG_ERROR:
			oss << "The sync failed: "
			    << cb->m_priv->osync_error_print(&status->error);
			error_event = true;
			break;
		}

		// call the status handler
		if( oss.str().size() )
			cb->m_status->EngineStatus(oss.str(),
				error_event,
				status->type == ENG_PREV_UNCLEAN);
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

void mapping_status(OSyncMappingUpdate *status, void *cbdata)
{
	CallbackBundle22 *cb = (CallbackBundle22*) cbdata;

	try {
		ostringstream oss;
		bool error_event = false;

		switch( status->type )
		{
		case MAPPING_SOLVED:
			oss << "Mapping solved";
			break;
		case MAPPING_SYNCED:
			oss << "Mapping Synced";
			break;
		case MAPPING_WRITE_ERROR:
			error_event = true;
			oss << "Mapping Write Error: "
			    << cb->m_priv->osync_error_print(&status->error);
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

void conflict_handler(OSyncEngine *engine, OSyncMapping *mapping, void *cbdata)
{
	CallbackBundle22 *cb = (CallbackBundle22*) cbdata;

	try {
		// build the SyncConflict object
		SyncConflict22Private scp(cb->m_priv, engine, mapping);
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


/////////////////////////////////////////////////////////////////////////////
// OpenSync22 - public members

bool OpenSync22::symbols_loaded = false;

OpenSync22::OpenSync22()
{
	if( !Open("libosengine.so.0") )
		throw DlError("Can't dlopen libosengine.so.0");

	// the symbol table is now thoroughly polluted...
	symbols_loaded = true;

	// store locally in case of constructor exception in LoadSym
	std::auto_ptr<OpenSync22Private> p(new OpenSync22Private(*this));

	// load all required symbols...
	// we don't need to use try/catch here, since the base
	// class destructor will clean up for us if LoadSym() throws
	LoadSym(p->osync_get_version, "osync_get_version");
	LoadSym(p->osync_env_new, "osync_env_new");
	LoadSym(p->osync_env_free, "osync_env_free");
	LoadSym(p->osync_env_set_option, "osync_env_set_option");
	LoadSym(p->osync_env_finalize, "osync_env_finalize");
	LoadSym(p->osync_env_num_plugins, "osync_env_num_plugins");
	LoadSym(p->osync_env_nth_plugin, "osync_env_nth_plugin");
	LoadSym(p->osync_plugin_get_name, "osync_plugin_get_name");
	LoadSym(p->osync_error_free, "osync_error_free");
	LoadSym(p->osync_error_print, "osync_error_print");
	LoadSym(p->osync_env_initialize, "osync_env_initialize");
	LoadSym(p->osync_env_num_groups, "osync_env_num_groups");
	LoadSym(p->osync_env_nth_group, "osync_env_nth_group");
	LoadSym(p->osync_group_get_name, "osync_group_get_name");
	LoadSym(p->osync_env_find_group, "osync_env_find_group");
	LoadSym(p->osync_group_num_members, "osync_group_num_members");
	LoadSym(p->osync_group_nth_member, "osync_group_nth_member");
	LoadSym(p->osync_member_get_id, "osync_member_get_id");
	LoadSym(p->osync_member_get_pluginname, "osync_member_get_pluginname");
	LoadSym(p->osync_conv_env_new, "osync_conv_env_new");
	LoadSym(p->osync_conv_env_free, "osync_conv_env_free");
	LoadSym(p->osync_conv_num_objtypes, "osync_conv_num_objtypes");
	LoadSym(p->osync_conv_nth_objtype, "osync_conv_nth_objtype");
	LoadSym(p->osync_conv_num_objformats, "osync_conv_num_objformats");
	LoadSym(p->osync_conv_nth_objformat, "osync_conv_nth_objformat");
	LoadSym(p->osync_objformat_get_name, "osync_objformat_get_name");
	LoadSym(p->osync_objtype_get_name, "osync_objtype_get_name");
	LoadSym(p->osync_group_new, "osync_group_new");
	LoadSym(p->osync_group_set_name, "osync_group_set_name");
	LoadSym(p->osync_group_save, "osync_group_save");
	LoadSym(p->osync_group_delete, "osync_group_delete");
	LoadSym(p->osync_member_new, "osync_member_new");
	LoadSym(p->osync_member_instance_plugin,"osync_member_instance_plugin");
	LoadSym(p->osync_member_save, "osync_member_save");
	LoadSym(p->osync_member_from_id, "osync_member_from_id");
	LoadSym(p->osync_member_need_config, "osync_member_need_config");
	LoadSym(p->osync_member_get_config_or_default,
					"osync_member_get_config_or_default");
	LoadSym(p->osync_member_set_config, "osync_member_set_config");
	LoadSym(p->osengine_mapping_ignore_supported,
					"osengine_mapping_ignore_supported");
	LoadSym(p->osengine_mapping_check_timestamps,
					"osengine_mapping_check_timestamps");
	LoadSym(p->osengine_mapping_nth_change, "osengine_mapping_nth_change");
	LoadSym(p->osengine_mapping_solve, "osengine_mapping_solve");
	LoadSym(p->osengine_mapping_duplicate, "osengine_mapping_duplicate");
	LoadSym(p->osengine_mapping_ignore_conflict,
					"osengine_mapping_ignore_conflict");
	LoadSym(p->osengine_mapping_solve_latest,
					"osengine_mapping_solve_latest");
	LoadSym(p->osync_change_get_uid, "osync_change_get_uid");
	LoadSym(p->osengine_init, "osengine_init");
	LoadSym(p->osync_change_get_member, "osync_change_get_member");
	LoadSym(p->osync_change_get_datasize, "osync_change_get_datasize");
	LoadSym(p->osengine_new, "osengine_new");
	LoadSym(p->osengine_free, "osengine_free");
	LoadSym(p->osengine_finalize, "osengine_finalize");
	LoadSym(p->osengine_sync_and_block, "osengine_sync_and_block");
	LoadSym(p->osengine_set_memberstatus_callback,
					"osengine_set_memberstatus_callback");
	LoadSym(p->osengine_set_changestatus_callback,
					"osengine_set_changestatus_callback");
	LoadSym(p->osengine_set_enginestatus_callback,
					"osengine_set_enginestatus_callback");
	LoadSym(p->osengine_set_mappingstatus_callback,
					"osengine_set_mappingstatus_callback");
	LoadSym(p->osengine_set_conflict_callback,
					"osengine_set_conflict_callback");
	LoadSym(p->osengine_mapping_num_changes,
					"osengine_mapping_num_changes");
	LoadSym(p->osync_change_get_changetype, "osync_change_get_changetype");
	LoadSym(p->osync_change_get_printable, "osync_change_get_printable");

	// do common initialization of opensync environment
	SetupEnvironment(p.get());

	// this pointer is ours now
	m_priv = p.release();
}

OpenSync22::~OpenSync22()
{
	// clean up opensync environment, closing plugins, etc.
	if( m_priv->env ) {
		m_priv->osync_env_finalize(m_priv->env, NULL);
		m_priv->osync_env_free(m_priv->env);
	}

	// free private class data
	delete m_priv;
	m_priv = 0;
}

void OpenSync22::SetupEnvironment(OpenSync22Private *p)
{
	// we are fully responsible for this pointer, since
	// we run inside the constructor... only on success
	// will responsibility be transferred to the destructor
	p->env = p->osync_env_new();
	if( !p->env )
		throw std::runtime_error("Error allocating opensync 0.22 environment");

	p->osync_env_set_option(p->env, "GROUPS_DIRECTORY", NULL);
	p->osync_env_set_option(p->env, "LOAD_GROUPS", "TRUE");
	p->osync_env_set_option(p->env, "LOAD_PLUGINS", "TRUE");
	p->osync_env_set_option(p->env, "LOAD_FORMATS", "TRUE");

	OSyncError *error = NULL;
	if( !p->osync_env_initialize(p->env, &error) ) {
		// grab error message
		std::runtime_error err(m_priv->osync_error_print(&error));

		// cleanup
		p->osync_error_free(&error);
		p->osync_env_free(m_priv->env);

		// throw
		throw err;
	}
}

const char* OpenSync22::GetVersion() const
{
	return m_priv->osync_get_version();
}

const char* OpenSync22::GetEngineName() const
{
	return "0.22";
}

void OpenSync22::GetPluginNames(string_list_type &plugins)
{
	// start fresh
	plugins.clear();

	OSyncPlugin *p;
	for( int i = 0; i < m_priv->osync_env_num_plugins(m_priv->env); i++) {
		p = m_priv->osync_env_nth_plugin(m_priv->env, i);
		plugins.push_back(m_priv->osync_plugin_get_name(p));
	}
}

void OpenSync22::GetFormats(format_list_type &formats)
{
	// start fresh
	formats.clear();

	// cycle through all object types and simulate a 0.4x-like
	// list based on the attached formats

	OSyncFormatEnv *fenv = m_priv->osync_conv_env_new(m_priv->env);
	if( !fenv ) {
		throw std::runtime_error("GetFormats(): Unable to load format environment in GetFormats (22)");
	}

	for( int i = 0; i < m_priv->osync_conv_num_objtypes(fenv); i++ ) {
		OSyncObjType *type = m_priv->osync_conv_nth_objtype(fenv, i);

		for( int i = 0; i < m_priv->osync_conv_num_objformats(type); i++ ) {
			OSyncObjFormat *format = m_priv->osync_conv_nth_objformat(type, i);
			const char *objformat_name = m_priv->osync_objformat_get_name(format);

			if( !formats.Find(objformat_name) ) {
				Format new_format;
				new_format.name = objformat_name;
				new_format.object_type = m_priv->osync_objtype_get_name(type);
				formats.push_back(new_format);
			}
		}
	}

	m_priv->osync_conv_env_free(fenv);
}

void OpenSync22::GetGroupNames(string_list_type &groups)
{
	// start fresh
	groups.clear();

	OSyncGroup *g;
	for( int i = 0; i < m_priv->osync_env_num_groups(m_priv->env); i++ ) {
		g = m_priv->osync_env_nth_group(m_priv->env, i);
		groups.push_back(m_priv->osync_group_get_name(g));
	}
}

void OpenSync22::GetMembers(const std::string &group_name,
			member_list_type &members)
{
	// start fresh
	members.clear();

	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group ) {
		throw std::runtime_error("GetMembers(): Unable to find group with name: " + group_name);
	}

	for( int i = 0; i < m_priv->osync_group_num_members(group); i++ ) {
		OSyncMember *member = m_priv->osync_group_nth_member(group, i);

		Member new_member;

		new_member.group_name = group_name;
		new_member.id = m_priv->osync_member_get_id(member);
		new_member.plugin_name = m_priv->osync_member_get_pluginname(member);

		// we are switching opensync's long long int ID to
		// our long... to double check they are equal after
		// the conversion
		if( new_member.id != m_priv->osync_member_get_id(member) ) {
			throw std::logic_error("GetMembers(): OpenSync's member ID is too large to fit in OpenSyncAPI (22)");
		}

		// add to member list
		members.push_back(new_member);
	}
}

void OpenSync22::AddGroup(const std::string &group_name)
{
	if( m_priv->osync_env_find_group(m_priv->env, group_name.c_str()) )
		throw std::runtime_error("AddGroup(): Group already exists: " + group_name);

	OSyncGroup *group = m_priv->osync_group_new(m_priv->env);
	m_priv->osync_group_set_name(group, group_name.c_str());

	OSyncError *error = NULL;
	if( !m_priv->osync_group_save(group, &error) ) {
		// grab error message
		std::runtime_error err(string("AddGroup(): Unable to save group: ") + m_priv->osync_error_print(&error));

		// cleanup
		m_priv->osync_error_free(&error);

		throw err;
	}
}

void OpenSync22::DeleteGroup(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("DeleteGroup(): Group not found: " + group_name);

	OSyncError *error = NULL;
	if( !m_priv->osync_group_delete(group, &error) ) {
		std::runtime_error err(string("DeleteGroup(): Unable to delete group: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}
}

Converter& OpenSync22::GetConverter()
{
	return m_priv->converter;
}

long OpenSync22::AddMember(const std::string &group_name,
			const std::string &plugin_name,
			const std::string &member_name)
{
	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("AddMember(): Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_new(group);

	OSyncError *error = NULL;
	if( !m_priv->osync_member_instance_plugin(member, plugin_name.c_str(), &error) ) {
		std::runtime_error err(string("AddMember(): Unable to connect plugin with member: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}

	if( !m_priv->osync_member_save(member, &error) ) {
		std::runtime_error err(string("AddMember(): Unable to save member: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}

	return m_priv->osync_member_get_id(member);
}

bool OpenSync22::IsConfigurable(const std::string &group_name,
				long member_id)
{
	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_from_id(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "IsConfigurable(): Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	OSyncConfigurationTypes type = NO_CONFIGURATION;
	OSyncError *error = NULL;
	if( !m_priv->osync_member_need_config(member, &type, &error) ) {
		std::runtime_error err(string("Unable to determine needed config: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}

	return type != NO_CONFIGURATION;
}

std::string OpenSync22::GetConfiguration(const std::string &group_name,
					long member_id)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "GetConfiguration(): Member " << member_id << " of group '" << group_name << "' does not accept configuration.";
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("GetConfiguration(): Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_from_id(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "GetConfiguration(): Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	OSyncError *error = NULL;
	char *data = NULL;
	int size = 0;
	if( !m_priv->osync_member_get_config_or_default(member, &data, &size, &error)) {
		std::runtime_error err(string("GetConfiguration(): Unable to retrieve config: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}

	std::string config(data, size);
	g_free(data);

	return config;
}

void OpenSync22::SetConfiguration(const std::string &group_name,
				long member_id,
				const std::string &config_data)
{
	if( !IsConfigurable(group_name, member_id) ) {
		ostringstream oss;
		oss << "SetConfiguration(): Member " << member_id << " of group '" << group_name << "' does not accept configuration.";
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("SetConfiguration(): Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_from_id(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "SetConfiguration(): Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	m_priv->osync_member_set_config(member, config_data.c_str(), config_data.size());

	OSyncError *error = NULL;
	if( !m_priv->osync_member_save(member, &error) ) {
		std::runtime_error err(string("SetConfiguration(): Unable to save member's config: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}
}

void OpenSync22::Discover(const std::string &group_name)
{
	// Discover is a successful noop on 0.22
}

void OpenSync22::Sync(const std::string &group_name,
			SyncStatus &status_callback)
{
	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("Sync(): Group not found: " + group_name);

	OSyncError *error = NULL;
	EngineHandle engine(m_priv->osengine_free);
	engine = m_priv->osengine_new(group, &error);
	if( !engine.get() ) {
		std::ostringstream oss;
		oss << "Error while synchronizing: "
		    << m_priv->osync_error_print(&error);
		m_priv->osync_error_free(&error);
		throw std::runtime_error(oss.str());
	}

	CallbackBundle22 cbdata(m_priv, status_callback);

	m_priv->osengine_set_memberstatus_callback(engine.get(),
					&member_status, &cbdata);
	m_priv->osengine_set_changestatus_callback(engine.get(),
					&entry_status, &cbdata);
	m_priv->osengine_set_enginestatus_callback(engine.get(),
					&engine_status, &cbdata);
	m_priv->osengine_set_mappingstatus_callback(engine.get(),
					&mapping_status, &cbdata);
	m_priv->osengine_set_conflict_callback(engine.get(),
					&conflict_handler, &cbdata);

	if( !m_priv->osengine_init(engine.get(), &error) ) {
		ostringstream oss;
		oss << "Error initializing osengine: "
		    << m_priv->osync_error_print(&error);
		m_priv->osync_error_free(&error);
		throw std::runtime_error(oss.str());
	}

	// successfully initialized, so finalize must be called
	EngineHandle initialized_engine(m_priv->osengine_finalize);
	initialized_engine = engine.get();

	if( !m_priv->osengine_sync_and_block(engine.get(), &error) ) {
		ostringstream oss;
		oss << "Error during sync: "
		    << m_priv->osync_error_print(&error);
		m_priv->osync_error_free(&error);
		throw std::runtime_error(oss.str());
	}
}

} // namespace OpenSync

