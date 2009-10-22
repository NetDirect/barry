///
/// \file	os22.cc
///		Wrapper class for opensync 0.22 syncing behaviour
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

    With code from msynctool: (GPL v2+)
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
#include <barry/barry.h>
#include <memory>
#include <glib.h>

#include <../opensync-1.0/opensync/opensync.h>
#include <../opensync-1.0/osengine/engine.h>

using namespace std;

namespace OpenSync {

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

	// data pointers
	OSyncEnv *env;

	OpenSync22Private()
		: env(0)
	{
	}
};

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
	std::auto_ptr<OpenSync22Private> p(new OpenSync22Private);

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
		throw std::runtime_error("Unable to load format environment in GetFormats (22)");
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
		throw std::runtime_error("Unable to find group with name: " + group_name);
	}

	for( int i = 0; i < m_priv->osync_group_num_members(group); i++ ) {
		OSyncMember *member = m_priv->osync_group_nth_member(group, i);

		Member new_member;

		new_member.id = m_priv->osync_member_get_id(member);
		new_member.plugin_name = m_priv->osync_member_get_pluginname(member);

		// we are switching opensync's long long int ID to
		// our long... to double check they are equal after
		// the conversion
		if( new_member.id != m_priv->osync_member_get_id(member) ) {
			throw std::logic_error("OpenSync's member ID is too large to fit in OpenSyncAPI (22)");
		}

		// add to member list
		members.push_back(new_member);
	}
}

void OpenSync22::AddGroup(const std::string &group_name)
{
	if( m_priv->osync_env_find_group(m_priv->env, group_name.c_str()) )
		throw std::runtime_error("Group already exists: " + group_name);

	OSyncGroup *group = m_priv->osync_group_new(m_priv->env);
	m_priv->osync_group_set_name(group, group_name.c_str());

	OSyncError *error = NULL;
	if( !m_priv->osync_group_save(group, &error) ) {
		// grab error message
		std::runtime_error err(string("Unable to save group: ") + m_priv->osync_error_print(&error));

		// cleanup
		m_priv->osync_error_free(&error);

		throw err;
	}
}

void OpenSync22::DeleteGroup(const std::string &group_name)
{
	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncError *error = NULL;
	if( !m_priv->osync_group_delete(group, &error) ) {
		std::runtime_error err(string("Unable to delete group: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}
}

void OpenSync22::AddMember(const std::string &group_name,
			const std::string &plugin_name,
			const std::string &member_name)
{
	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_new(group);

	OSyncError *error = NULL;
	if( !m_priv->osync_member_instance_plugin(member, plugin_name.c_str(), &error) ) {
		std::runtime_error err(string("Unable to connect plugin with member: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}

	if( !m_priv->osync_member_save(member, &error) ) {
		std::runtime_error err(string("Unable to save member: ") + m_priv->osync_error_print(&error));
		m_priv->osync_error_free(&error);
		throw err;
	}
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
		oss << "Member " << member_id << " not found.";
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
		oss << "Member " << member_id << " of group '" << group_name << "' does not accept configuration.";
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_from_id(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	OSyncError *error = NULL;
	char *data = NULL;
	int size = 0;
	if( !m_priv->osync_member_get_config_or_default(member, &data, &size, &error)) {
		std::runtime_error err(string("Unable to retrieve config: ") + m_priv->osync_error_print(&error));
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
		oss << "Member " << member_id << " of group '" << group_name << "' does not accept configuration.";
		throw std::runtime_error(oss.str());
	}

	OSyncGroup *group = m_priv->osync_env_find_group(m_priv->env, group_name.c_str());
	if( !group )
		throw std::runtime_error("Group not found: " + group_name);

	OSyncMember *member = m_priv->osync_member_from_id(group, member_id);
	if( !member ) {
		ostringstream oss;
		oss << "Member " << member_id << " not found.";
		throw std::runtime_error(oss.str());
	}

	m_priv->osync_member_set_config(member, config_data.c_str(), config_data.size());

	OSyncError *error = NULL;
	if( !m_priv->osync_member_save(member, &error) ) {
		std::runtime_error err(string("Unable to save member's config: ") + m_priv->osync_error_print(&error));
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
}

} // namespace OpenSync


/////////////////////////////////////////////////////////////////////////////
// msynctool source code

#if 0

/*
 * msynctool - A command line client for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
 
#include <opensync/opensync.h>
#include <osengine/engine.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>
#include "config.h"
#include <errno.h>

typedef enum MSyncResolution {
	MSYNC_RESOLUTION_UNKNOWN,
	MSYNC_RESOLUTION_DUPLICATE,
	MSYNC_RESOLUTION_IGNORE,
	MSYNC_RESOLUTION_NEWER,
	MSYNC_RESOLUTION_SELECT
} MSyncResolution;

osync_bool manual = FALSE;
MSyncResolution conflict = MSYNC_RESOLUTION_UNKNOWN;
int winner = 0;

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s [options]\n\n", name);
  fprintf (stderr, "--listgroups\t\t\t\tLists all groups\n");
  fprintf (stderr, "--listplugins\t\t\t\tLists all plugins\n");
  fprintf (stderr, "--listobjects\t\t\t\tLists all object types that the\n");
  fprintf (stderr, "\t\t\t\t\t  engine understands\n");
  fprintf (stderr, "--showformats <objtype>\t\t\tLists all formats that a object\n");
  fprintf (stderr, "\t\t\t\t\t  type can have\n");
  fprintf (stderr, "--showgroup <groupname>\t\t\tLists all members of the group\n");
  fprintf (stderr, "--sync <groupname>\t\t\tSync all members in a group\n");
  fprintf (stderr, "--filter-objtype <objtype>\t\tFilter out object type\n");
  fprintf (stderr, "--slow-sync <objtype>\t\t\tPerform a slow-sync of all members\n");
  fprintf (stderr, "\t\t\t\t\t  in the group\n");
  fprintf (stderr, "[--wait]\t\t\t\tDon't immediately start to sync, but\n");
  fprintf (stderr, "\t\t\t\t\t  wait for a client to initialize the\n");
  fprintf (stderr, "\t\t\t\t\t  sync\n");
  fprintf (stderr, "[--multi]\t\t\t\tRepeat to wait for sync alerts\n");
  fprintf (stderr, "--addgroup <groupname>\t\t\tAdd a new group\n");
  fprintf (stderr, "--delgroup <groupname>\t\t\tDelete the given group\n");
  fprintf (stderr, "--addmember <groupname> <plugintype>\tAdd a member to the group\n");
  fprintf (stderr, "--configure <groupname> <memberid>\tConfigure a member.  Memberid as\n");
  fprintf (stderr, "\t\t\t\t\t  returned by --showgroup\n");
  fprintf (stderr, "[--manual]\t\t\t\tMake manual engine iterations. Only\n");
  fprintf (stderr, "\t\t\t\t\tfor debugging.\n");
  fprintf (stderr, "[--configdir]\t\t\t\tUse a different configdir than\n");
  fprintf (stderr, "\t\t\t\t\t  ~/.opensync\n");
  fprintf (stderr, "[--conflict 1-9/d/i/n]\t\t\tResolve all conflicts as side [1-9]\n");
  fprintf (stderr, "\t\t\t\t\twins, [d]uplicate, [i]gnore, or\n");
  fprintf (stderr, "\t\t\t\t\t  keep [n]ewer\n");
  exit (ecode);
}

typedef enum  {
	NONE = 0,
	LISTGROUPS = 1,
	LISTPLUGINS = 2,
	SHOWGROUP = 3,
	SYNC = 4,
	ADDMEMBER = 5,
	CONFIGURE = 6,
	ADDGROUP = 7,
	LISTOBJECTS = 8,
	SHOWFORMATS = 9,
	DELGROUP = 10,
	GETVERSION = 11
} ToolAction;

void display_groups(OSyncEnv *osync)
{
	int i;
	OSyncGroup *group;
	
	printf("Available groups:\n");
	
	for (i = 0; i < osync_env_num_groups(osync); i++) {
		group = osync_env_nth_group(osync, i);
		printf("%s\n", osync_group_get_name(group));
	}
}

void display_objtypes(OSyncEnv *osync)
{
	int i;
	OSyncFormatEnv *env = osync_conv_env_new(osync);
	OSyncError *error = NULL;
	if (!env) {
		fprintf(stderr, "Unable to load format environment: %s\n", osync_error_print(&error));
		exit(1);
	}
	
	printf("Available objtypes:\n");
	
	for (i = 0; i < osync_conv_num_objtypes(env); i++) {
		OSyncObjType *type = osync_conv_nth_objtype(env, i);
		printf("%s\n", osync_objtype_get_name(type));
	}
}

void member_status(OSyncMemberUpdate *status, void *user_data)
{
	switch (status->type) {
		case MEMBER_CONNECTED:
			printf("Member %lli of type %s just connected\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member));
			break;
		case MEMBER_DISCONNECTED:
			printf("Member %lli of type %s just disconnected\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member));
			break;
		case MEMBER_SENT_CHANGES:
			printf("Member %lli of type %s just sent all changes\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member));
			break;
		case MEMBER_COMMITTED_ALL:
			printf("Member %lli of type %s committed all changes.\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member));
			break;
		case MEMBER_CONNECT_ERROR:
			printf("Member %lli of type %s had an error while connecting: %s\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member), osync_error_print(&(status->error)));
			break;
		case MEMBER_GET_CHANGES_ERROR:
			printf("Member %lli of type %s had an error while getting changes: %s\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member), osync_error_print(&(status->error)));
			break;
		case MEMBER_SYNC_DONE_ERROR:
			printf("Member %lli of type %s had an error while calling sync done: %s\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member), osync_error_print(&(status->error)));
			break;
		case MEMBER_DISCONNECT_ERROR:
			printf("Member %lli of type %s had an error while disconnecting: %s\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member), osync_error_print(&(status->error)));
			break;
		case MEMBER_COMMITTED_ALL_ERROR:
			printf("Member %lli of type %s had an error while commiting changes: %s\n", osync_member_get_id(status->member), osync_member_get_pluginname(status->member), osync_error_print(&(status->error)));
			break;
	}
}

void mapping_status(OSyncMappingUpdate *status, void *user_data)
{
	switch (status->type) {
		case MAPPING_SOLVED:
			printf("Mapping solved\n");
			break;
		case MAPPING_SYNCED:
			printf("Mapping Synced\n");
			break;
		case MAPPING_WRITE_ERROR:
			printf("Mapping Write Error: %s\n", osync_error_print(&(status->error)));
			break;
	}
}

void engine_status(OSyncEngine *engine, OSyncEngineUpdate *status, void *user_data)
{
	switch (status->type) {
		case ENG_PREV_UNCLEAN:
			printf("The previous synchronization was unclean. Slow-syncing\n");
			break;
		case ENG_ENDPHASE_CON:
			printf("All clients connected or error\n");
			break;
		case ENG_END_CONFLICTS:
			printf("All conflicts have been reported\n");
			break;
		case ENG_ENDPHASE_READ:
			printf("All clients sent changes or error\n");
			break;
		case ENG_ENDPHASE_WRITE:
			printf("All clients have written\n");
			break;
		case ENG_ENDPHASE_DISCON:
			printf("All clients have disconnected\n");
			break;
		case ENG_SYNC_SUCCESSFULL:
			printf("The sync was successful\n");
			break;
		case ENG_ERROR:
			printf("The sync failed: %s\n", osync_error_print(&(status->error)));
			break;
	}
}

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

void entry_status(OSyncEngine *engine, OSyncChangeUpdate *status, void *user_data)
{
	OSyncMember *member = osync_change_get_member(status->change);

	switch (status->type) {
		case CHANGE_RECEIVED_INFO:
			printf("Received an entry %s without data from member %i (%s). Changetype %s\n",
					osync_change_get_uid(status->change), 
					status->member_id,
					osync_member_get_pluginname(member),
					OSyncChangeType2String(osync_change_get_changetype(status->change)));
			break;
		case CHANGE_RECEIVED:
			printf("Received an entry %s with data of size %i from member %i (%s). Changetype %s\n",
					osync_change_get_uid(status->change),
					osync_change_get_datasize(status->change),
					status->member_id,
					osync_member_get_pluginname(member),
					OSyncChangeType2String(osync_change_get_changetype(status->change)));
			break;
		case CHANGE_SENT:
			printf("Sent an entry %s of size %i to member %i (%s). Changetype %s\n",
					osync_change_get_uid(status->change),
					osync_change_get_datasize(status->change),
					status->member_id,
					osync_member_get_pluginname(member),
					OSyncChangeType2String(osync_change_get_changetype(status->change)));
			break;
		case CHANGE_WRITE_ERROR:
			printf("Error writing entry %s to member %i (%s): %s\n",
					osync_change_get_uid(status->change),
					status->member_id,
					osync_member_get_pluginname(member),
					osync_error_print(&(status->error)));
			break;
		case CHANGE_RECV_ERROR:
			printf("Error reading entry %s from member %i (%s): %s\n", 
					osync_change_get_uid(status->change), 
					status->member_id, 
					osync_member_get_pluginname(member),
					osync_error_print(&(status->error)));
			break;
	}
}

void conflict_handler(OSyncEngine *engine, OSyncMapping *mapping, void *user_data)
{
	OSyncError	*error = NULL;
	MSyncResolution res = MSYNC_RESOLUTION_UNKNOWN;
	OSyncChange *change = NULL;
	
	printf("Conflict for Mapping %p: ", mapping);
	fflush(stdout);
	
	if (conflict != MSYNC_RESOLUTION_UNKNOWN)
		res = conflict;
	
	/* Check if the original value for the winning side was valid */
	if (conflict == MSYNC_RESOLUTION_SELECT) {
		if (!osengine_mapping_nth_change(mapping, winner)) {
			printf("Unable to find change #%i\n", winner + 1);
			res = MSYNC_RESOLUTION_UNKNOWN;
		}
	}
				
	if (res == MSYNC_RESOLUTION_UNKNOWN) {
		int i = 0;
		for (i = 0; i < osengine_mapping_num_changes(mapping); i++) {
			OSyncChange *change = osengine_mapping_nth_change(mapping, i);
			if (osync_change_get_changetype(change) != CHANGE_UNKNOWN) {
				char *printable = osync_change_get_printable(change);
				printf("\nEntry %i:\nUID: %s\n%s\n", i+1, osync_change_get_uid(change), printable);
				g_free(printable);
			}
		}
		
		while (res == MSYNC_RESOLUTION_UNKNOWN) {
			int supported_ignore = osengine_mapping_ignore_supported(engine, mapping);
			int supported_newer = osengine_mapping_check_timestamps(engine, mapping, NULL); 

			printf("\nWhich entry do you want to use? [1-9] To select a side, [D]uplicate");

			if (supported_ignore)
				printf(", [I]gnore");

			if (supported_newer)
				printf(", Keep [N]ewer");

			printf(": ");

			fflush(stdout);
			int inp = getchar();
			while (inp != '\n' && getchar() != '\n');
			
			if (inp == 'D' || inp == 'd')
				res = MSYNC_RESOLUTION_DUPLICATE;
			else if ((inp == 'i' || inp == 'I') && supported_ignore)
				res = MSYNC_RESOLUTION_IGNORE;
			else if ((inp == 'n' || inp == 'N') && supported_newer)
				res = MSYNC_RESOLUTION_NEWER;
			else if (strchr("123456789", inp) != NULL) {
				char inpbuf[2];
				inpbuf[0] = inp;
				inpbuf[1] = 0;
				
				winner = atoi(inpbuf) - 1;
				if (winner >= 0) {
					if (!osengine_mapping_nth_change(mapping, winner))
						printf("Unable to find change #%i\n", winner + 1);
					else
						res = MSYNC_RESOLUTION_SELECT;
				}
			}
		}
	}
		
	/* Did we get default conflict resolution ? */
	switch (res) {
		case MSYNC_RESOLUTION_DUPLICATE:
			printf("Mapping duplicated\n");
			osengine_mapping_duplicate(engine, mapping);
			return;
		case MSYNC_RESOLUTION_IGNORE:
			printf("Conflict ignored\n");
			if (!osengine_mapping_ignore_conflict(engine, mapping, &error)) {
                                printf("Conflict not ignored: %s\n", osync_error_print(&error));
                                osync_error_free(&error);
                                break;
                        }
			return;
		case MSYNC_RESOLUTION_NEWER:
			printf("Newest entry used\n");
			if (!osengine_mapping_solve_latest(engine, mapping, &error)) {
				printf("Conflict not resolved: %s\n", osync_error_print(&error));
				osync_error_free(&error);
				break;
			}
			return;
		case MSYNC_RESOLUTION_SELECT:
			printf("Overwriting conflict\n");
			
			change = osengine_mapping_nth_change(mapping, winner);
				
			g_assert(change);
			osengine_mapping_solve(engine, mapping, change);
			return;
		case MSYNC_RESOLUTION_UNKNOWN:
			g_assert_not_reached();
	}
}

static void synchronize(OSyncEnv *osync, const char *groupname, osync_bool wait, osync_bool multi, GList *objtypes, GList *filterobjs)
{
	OSyncError *error = NULL;
	
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	if (!group) {
		printf("Unable to find group with name \"%s\"\n", groupname);
		return;
	}
	
	printf("Synchronizing group \"%s\" %s\n", osync_group_get_name(group), objtypes ? "[slow sync]" : "");

	GList *o = NULL;
	for (o = objtypes; o; o = o->next) {
		osync_group_set_slow_sync(group, o->data, TRUE);
	}

	for (o = filterobjs; o; o = o->next) {
		osync_group_set_objtype_enabled(group, o->data, 0);
	}

	OSyncEngine *engine = osengine_new(group, &error);
	if (!engine)
		goto error;
	
	
	osengine_set_memberstatus_callback(engine, member_status, NULL);
	osengine_set_changestatus_callback(engine, entry_status, NULL);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	osengine_set_mappingstatus_callback(engine, mapping_status, NULL);
	osengine_set_conflict_callback(engine, conflict_handler, NULL);
	//osengine_set_message_callback(engine, plgmsg_function, "palm_uid_mismatch", NULL);
	//osengine_flag_only_info(engine);
	if (manual)
		osengine_flag_manual(engine);
	
	if (!osengine_init(engine, &error))
		goto error_free;
	
	if (!wait) {
		if (!manual) {
			if (!osengine_sync_and_block(engine, &error))
				goto error_finalize;
		} else {
			if (!osengine_synchronize(engine, &error))
				goto error_finalize;
				
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
	}

	osengine_finalize(engine);
	osengine_free(engine);
	return;

error_finalize:
	osengine_finalize(engine);
error_free:
	osengine_free(engine);
error:
	printf("Error while synchronizing: %s\n", osync_error_print(&error));
	osync_error_free(&error);
}

void show_formats(OSyncEnv *osync, char *objtype)
{
	int i;
	
	OSyncFormatEnv *env = osync_conv_env_new(osync);
	OSyncError *error = NULL;
	if (!env) {
		fprintf(stderr, "Unable to load format environment: %s\n", osync_error_print(&error));
		exit(1);
	}
	
	OSyncObjType *type = osync_conv_find_objtype(env, objtype);
	
	if (!type) {
		printf("Unable to find objtype with name %s\n", objtype);
		return;
	}
	
	printf("Object Type: %s\n", osync_objtype_get_name(type));
	
	for (i = 0; i < osync_conv_num_objformats(type); i++) {
		OSyncObjFormat *format = osync_conv_nth_objformat(type, i);
		printf("%s\n", osync_objformat_get_name(format));
	}
}

void show_group(OSyncEnv *osync, char *groupname)
{
	int i;
	OSyncMember *member = NULL;
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	OSyncError	*error = NULL;
	char		*data = NULL;
	int		size = 0;
	
	if (!group) {
		printf("Unable to find group with name %s\n", groupname);
		return;
	}
	
	printf("Groupname: %s\n", osync_group_get_name(group));
	
	for (i = 0; i < osync_group_num_members(group); i++) {
		member = osync_group_nth_member(group, i);
		printf("Member %lli: %s\n", osync_member_get_id(member), osync_member_get_pluginname(member));

		/*
		 * Print the current configuration
		 */
		if (!osync_member_get_config(member, &data, &size, &error)) {
			printf("\tNo Configuration found: %s\n", osync_error_print(&error));
			osync_error_free(&error);
		} else {
			printf("\tConfiguration : %s\n", data);
		}
	}
}

static osync_bool configure(OSyncEnv *osync, const char *groupname, const char *memberid, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p)", __func__, osync, groupname, memberid, error);
	char *data = NULL;
	int size = 0;
	
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	if (!group) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find group with name %s", groupname);
		goto error;
	}
	
	long long id = atoi(memberid);
	OSyncMember *member = osync_member_from_id(group, id);
	if (!member) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find member with id %s", memberid);
		goto error;
	}
	
	OSyncConfigurationTypes type = NO_CONFIGURATION;
	if (!osync_member_need_config(member, &type, error))
		goto error;
	
	if (type == NO_CONFIGURATION) {
		printf("This plugin has no options and does not need to be configured\n");
		osync_trace(TRACE_EXIT, "%s: No options", __func__);
		return TRUE;
	}
	
	if (!osync_member_get_config_or_default(member, &data, &size, error))
		goto error;
		
	char *tmpfile = g_strdup_printf("%s/msynctooltmp-XXXXXX", g_get_tmp_dir());
	int file = mkstemp(tmpfile);
	if (!file) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create temp file");
		goto error_free_data;
	}
	
	if (write(file, data, size) == -1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to write to temp file: %i", errno);
		goto error_close_file;
	}
	
	if (close(file) == -1)  {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to close temp file: %i", errno);
		goto error_free_data;
	}
	
	g_free(data);
	
	char *editor = NULL;
	if (!(editor = getenv("EDITOR"))) {
		if (g_file_test("/usr/bin/editor", G_FILE_TEST_IS_EXECUTABLE)) {
			editor = "/usr/bin/editor";
		} else {
			editor = getenv("VISUAL");
		}
	}
	
	char *editcmd = NULL;
	if (editor)
		editcmd = g_strdup_printf("%s %s", editor, tmpfile);
	else	
		editcmd = g_strdup_printf("vi %s", tmpfile);

	if (system(editcmd)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open editor. Aborting");
		g_free(editcmd);
		goto error_free_tmp;
	}

	g_free(editcmd);
	
	if (!osync_file_read(tmpfile, &data, &size, error))
		goto error_free_tmp;
	
	osync_member_set_config(member, data, size);
	g_free(data);
	
	if (!osync_member_save(member, error))
		goto error_free_tmp;
	
	if (remove(tmpfile) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to remove file %s", tmpfile);
		goto error_free_tmp;
	}
	
	g_free(tmpfile);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_close_file:
	close(file);
error_free_data:
	g_free(data);
error_free_tmp:
	g_free(tmpfile);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void display_plugins(OSyncEnv *osync)
{
	int i;
	OSyncPlugin *plugin;
	
	printf("Available plugins:\n");
	
	for (i = 0; i < osync_env_num_plugins(osync); i++) {
		plugin = osync_env_nth_plugin(osync, i);
		printf("%s\n", osync_plugin_get_name(plugin));
	}
}

void addmember(OSyncEnv *osync, char *groupname, char *pluginname)
{
	OSyncError *error = NULL;
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name %s\n", groupname);
		return;
	}
	
	OSyncMember *member = osync_member_new(group);
	if (!osync_member_instance_plugin(member, pluginname, &error)) {
		printf("Unable to instance plugin with name %s: %s\n", pluginname, osync_error_print(&error));
		osync_error_free(&error);
		return;
	}
	
	if (!osync_member_save(member, &error)) {
		printf("Unable to save member: %s\n", osync_error_print(&error));
		osync_error_free(&error);
	}
}

void delgroup(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_env_find_group(osync, groupname);
	
	if (!group) {
		printf("Unable to find group with name %s\n", groupname);
		return;
	}
	
	OSyncError *error = NULL;
	if (!osync_group_delete(group, &error)) {
		printf("Unable to delete group %s: %s\n", groupname, osync_error_print(&error));
		osync_error_free(&error);
	}
	
}

void addgroup(OSyncEnv *osync, char *groupname)
{
	OSyncGroup *group = osync_group_new(osync);
	osync_group_set_name(group, groupname);

	OSyncError *error = NULL;
	if (!osync_group_save(group, &error)) {
		printf("Unable to save group: %s\n", osync_error_print(&error));
		osync_error_free(&error);
	}
}

void show_version(void)
{
	printf("This is msynctool version \"%s\"\n", VERSION);
	printf("using OpenSync version \"%s\"\n", osync_get_version());
}

int main (int argc, char *argv[])
{
	int i;
	char *groupname = NULL;
	char *membername = NULL;
	char *pluginname = NULL;
	ToolAction action = NONE;
	manual = FALSE;
	conflict = MSYNC_RESOLUTION_UNKNOWN;
	osync_bool wait = FALSE;
	osync_bool multi = FALSE;
	char *configdir = NULL;
	GList *slow_objects = NULL;
	GList *filter_objects = NULL;
	char *objtype = NULL;
	
	const char *load_groups = "TRUE";
	const char *load_plugins = "TRUE";
	const char *load_formats = "TRUE";
	
	if (argc == 1)
		usage (argv[0], 1);
	
	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--listgroups")) {
			action = LISTGROUPS;
			load_plugins = "FALSE";
		} else if (!strcmp (arg, "--listplugins")) {
			action = LISTPLUGINS;
			load_formats = "FALSE";
			load_groups = "FALSE";
		} else if (!strcmp (arg, "--listobjects")) {
			action = LISTOBJECTS;
			load_plugins = "FALSE";
			load_groups = "FALSE";
		} else if (!strcmp (arg, "--showformats")) {
			action = SHOWFORMATS;
			load_plugins = "FALSE";
			load_groups = "FALSE";
			objtype = argv[i + 1];
			i++;
			if (!objtype)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--showgroup")) {
			action = SHOWGROUP;
			load_plugins = "TRUE";
			groupname = argv[i + 1];
			i++;
			if (!groupname)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--sync")) {
			action = SYNC;			
			groupname = argv[i + 1];
			i += 1;
			if (!groupname)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--filter-objtype")) {
			objtype = argv[i + 1];
			filter_objects = g_list_append(filter_objects, objtype);
			i += 1;
			if (action != SYNC || !objtype)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--slow-sync")) {
			objtype = argv[i + 1];
			slow_objects = g_list_append(slow_objects, objtype);
			i += 1;
			if (action != SYNC || !objtype)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--wait")) {
			wait = TRUE;
		} else if (!strcmp (arg, "--multi")) {
			multi = TRUE;
		} else if (!strcmp (arg, "--addmember")) {
			action = ADDMEMBER;
			groupname = argv[i + 1];
			pluginname = argv[i + 2];
			i += 2;
			if (!groupname || !pluginname)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--addgroup")) {
			action = ADDGROUP;
			load_plugins = "FALSE";
			load_formats = "FALSE";
			groupname = argv[i + 1];
			i += 1;
			if (!groupname)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--delgroup")) {
			action = DELGROUP;
			load_plugins = "FALSE";
			groupname = argv[i + 1];
			i += 1;
			if (!groupname)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--configure")) {
			action = CONFIGURE;
			groupname = argv[i + 1];
			membername = argv[i + 2];
			i += 2;
			if (!groupname || !membername)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--conflict")) {
			const char *conflictstr = argv[i + 1];
			i++;
			if (!conflictstr)
				usage (argv[0], 1);
			
			if (conflictstr[0] == 'd' || conflictstr[0] == 'D')
				conflict = MSYNC_RESOLUTION_DUPLICATE;
			else if (conflictstr[0] == 'i' || conflictstr[0] == 'I')
				conflict = MSYNC_RESOLUTION_IGNORE;
			else if (conflictstr[0] == 'n' || conflictstr[0] == 'N')
				conflict = MSYNC_RESOLUTION_NEWER;
			else if (strchr("123456789", conflictstr[0]) != NULL) {
				winner = atoi(conflictstr) - 1;
				if (winner < 0)
					usage (argv[0], 1);
				conflict = MSYNC_RESOLUTION_SELECT;
			} else
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--configdir")) {
			configdir = argv[i + 1];
			i++;
			if (!configdir)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--version")) {
			action = GETVERSION;
		} else if (!strcmp (arg, "--manual")) {
			manual = TRUE;
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else if (!strcmp (arg, "--")) {
			break;
		} else if (arg[0] == '-') {
			usage (argv[0], 1);
		} else {
			usage (argv[0], 1);
		}
	}
	
	OSyncEnv *osync = osync_env_new();
	osync_env_set_option(osync, "GROUPS_DIRECTORY", configdir);
	osync_env_set_option(osync, "LOAD_GROUPS", load_groups);
	osync_env_set_option(osync, "LOAD_PLUGINS", load_plugins);
	osync_env_set_option(osync, "LOAD_FORMATS", load_formats);
	
	OSyncError *error = NULL;
	if (!osync_env_initialize(osync, &error)) {
		osync_error_update(&error, "Unable to initialize environment: %s\n", osync_error_print(&error));
		goto error_free_env;
	}
	
	switch (action) {
		case LISTGROUPS:
			display_groups(osync);
			break;
		case LISTPLUGINS:
			display_plugins(osync);
			break;
		case LISTOBJECTS:
			display_objtypes(osync);
			break;
		case SHOWGROUP:
			show_group(osync, groupname);
			break;
		case SHOWFORMATS:
			show_formats(osync, objtype);
			break;
		case SYNC:
			synchronize(osync, groupname, wait, multi, slow_objects, filter_objects);
			break;
		case ADDMEMBER:
			addmember(osync, groupname, pluginname);
			break;
		case CONFIGURE:
			if (!configure(osync, groupname, membername, &error))
				goto error_finalize;
			break;
		case ADDGROUP:
			addgroup(osync, groupname);
			break;
		case DELGROUP:
			delgroup(osync, groupname);
			break;
		case GETVERSION:
			show_version();
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "No action given");
			goto error_finalize;
	}
	
	if (!osync_env_finalize(osync, &error)) {
		osync_error_update(&error, "Unable to finalize environment: %s\n", osync_error_print(&error));
		goto error_free_env;
	}
	
	osync_env_free(osync);
	
	return 0;

error_finalize:
	osync_env_finalize(osync, NULL);
error_free_env:
	osync_env_free(osync);
	fprintf(stderr, "%s\n", osync_error_print(&error));
	osync_error_free(&error);
	return -1;
}

#endif

