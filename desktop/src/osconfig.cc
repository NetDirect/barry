///
/// \file	osconfig.cc
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "osconfig.h"
#include "os40.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <algorithm>

using namespace std;

namespace OpenSync { namespace Config {

//////////////////////////////////////////////////////////////////////////////
// Evolution

void Evolution::SetIfExists(std::string &var, const std::string &dir)
{
	struct stat s;
	if( stat(dir.c_str(), &s) == 0 ) {
		if( S_ISDIR(s.st_mode) ) {
			var = dir;
		}
	}
}

bool Evolution::AutoDetect()
{
	struct passwd *pw = getpwuid(getuid());
	if( !pw )
		return false;

	string base = pw->pw_dir;
	base += "/.evolution/";

	string tail = "/local/system";

	SetIfExists(m_address_path, base + "addressbook" + tail);
	SetIfExists(m_calendar_path, base + "calendar" + tail);
	SetIfExists(m_tasks_path, base + "tasks" + tail);
	SetIfExists(m_memos_path, base + "memos" + tail);

	// first three are required
	return m_address_path.size() &&
		m_calendar_path.size() &&
		m_tasks_path.size();
}

//////////////////////////////////////////////////////////////////////////////
// Group class

Group::Group(const std::string &group_name,
		OpenSync::API &api,
		unsigned throw_mask)
	: m_group_name(group_name)
{
	Load(api, throw_mask);
}

Group::Group(const std::string &group_name)
	: m_group_name(group_name)
{
}

// Checks for OSCG_THROW_ON_NO_BARRY and OSCG_THROW_ON_MULTIPLE_BARRIES
void Group::BarryCheck(OpenSync::API &api,
			const std::string &group_name,
			const member_list_type &members,
			unsigned throw_mask)
{
	if( ! (throw_mask & (OSCG_THROW_ON_NO_BARRY | OSCG_THROW_ON_MULTIPLE_BARRIES) ) )
		return;		// nothing to do

	int found = 0;
	std::string barry_name = Config::Barry::PluginName(api);
	member_list_type::const_iterator b = members.begin(), e = members.end();
	for( ; b != e; ++b ) {
		if( b->plugin_name == barry_name )
			found++;
	}

	if( found == 0 && (throw_mask & OSCG_THROW_ON_NO_BARRY) ) {
		ostringstream oss;
		oss << "No Barry plugins found in group '" << group_name << "' and OSCG_THROW_ON_NO_BARRY is set.";
		throw LoadError(oss.str());
	}

	if( found > 1 && (throw_mask & OSCG_THROW_ON_MULTIPLE_BARRIES) ) {
		ostringstream oss;
		oss << "Found " << found << " Barry plugins in group '" << group_name << "' and OSCG_THROW_ON_MULTIPLE_BARRIES is set.";
		throw LoadError(oss.str());
	}
}

bool Group::GroupMatchesExistingConfig(OpenSync::API &api)
{
	member_list_type members;
	api.GetMembers(m_group_name, members);

	// what needs to match:
	//
	//	- number of connected Config::Plugin objects in our
	//		list must match number of members in config
	//	- each member ID must match along with the plugin_name
	//		of each member
	//

	// check totals match
	if( (unsigned) GetConnectedCount() != members.size() ) {
		barryverbose("Connected count of " << GetConnectedCount() << " does not match member count of " << members.size());
		return false;
	}

	// cycle through our own vector, matching against each member_id
	// in the members list
	const_iterator ci = begin(), ce = end();
	for( ; ci != ce; ++ci ) {
		if( (*ci)->GetMemberId() == -1 )
			continue;

		Member *m = members.Find( (*ci)->GetMemberId() );
		if( !m ) {
			barryverbose("Can't match member ID: " << (*ci)->GetMemberId() );
			return false;
		}

		if( m->plugin_name != (*ci)->GetPluginName(api) ) {
			barryverbose("Plugin names don't match: "
				<< m->plugin_name << ", "
				<< (*ci)->GetPluginName(api));
			return false;
		}
	}

	return true;
}

bool Group::HasUnsupportedPlugins() const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( (*b)->IsUnsupported() )
			return true;
	}
	return false;
}

bool Group::HasBarryPlugins() const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( (*b)->GetAppName() == Config::Barry::AppName() )
			return true;
	}
	return false;
}

bool Group::GroupExists(OpenSync::API &api) const
{
	// does m_group_name exist in the API list?
	string_list_type groups;
	api.GetGroupNames(groups);
	return std::find(groups.begin(), groups.end(), m_group_name) != groups.end();
}

bool Group::AllConfigured(OpenSync::API &api) const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( !(*b)->IsConfigured(api) )
			return false;
	}
	return true;
}

int Group::GetConnectedCount() const
{
	int count = 0;
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( (*b)->GetMemberId() != -1 )
			count++;
	}
	return count;
}

std::string Group::GetAppNames() const
{
	std::string names;

	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		std::string name = (*b)->GetAppName();
		if( name != Config::Barry::AppName() ) {
			if( names.size() )
				names += ", ";
			names += name;
		}
	}

	return names;
}

OpenSync::Config::Barry& Group::GetBarryPlugin()
{
	return const_cast<OpenSync::Config::Barry&> ( const_cast<const Group*> (this)->GetBarryPlugin() );
}

const OpenSync::Config::Barry& Group::GetBarryPlugin() const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( (*b)->GetAppName() == Config::Barry::AppName() )
			return dynamic_cast<const OpenSync::Config::Barry&> (*(*b));
	}

	// not found
	throw std::logic_error("No Barry Plugins in Group");
}

OpenSync::Config::Plugin* Group::GetNonBarryPlugin()
{
	return const_cast<OpenSync::Config::Plugin*> ( const_cast<const Group*> (this)->GetNonBarryPlugin() );
}

const OpenSync::Config::Plugin* Group::GetNonBarryPlugin() const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		if( (*b)->GetAppName() != Config::Barry::AppName() )
			return (*b).get();
	}

	return 0;
}

void Group::DisconnectMembers()
{
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		(*b)->SetMemberId(-1);
	}
}

void Group::Load(OpenSync::API &api, unsigned throw_mask)
{
	Load(m_group_name, api, throw_mask);
}

void Group::Load(const std::string &src_group_name,
		OpenSync::API &api,
		unsigned throw_mask)
{
	member_list_type members;
	api.GetMembers(src_group_name, members);

	BarryCheck(api, src_group_name, members, throw_mask);

	member_list_type::const_iterator b = members.begin(), e = members.end();
	for( ; b != e; ++b ) {
		Converter &converter = api.GetConverter();
		Converter::plugin_ptr p = converter.CreateAndLoadPlugin(*b);
		p->SetMemberId(b->id);

		if( p->IsUnsupported() && (throw_mask & OSCG_THROW_ON_UNSUPPORTED) ) {
			ostringstream oss;
			oss << "Unsupported plugin '" << b->plugin_name << "' in group '" << src_group_name << "' and OSCG_THROW_ON_UNSUPPORTED is set.";
			throw LoadError(oss.str());
		}

		// everything looks ok, add the plugin to our list
		push_back(p);
	}
}

void Group::ResetGroupName(const std::string &new_group_name)
{
	m_group_name = new_group_name;
}

void Group::AddPlugin(OpenSync::Config::Plugin *plugin)
{
	plugin_ptr pp(plugin);
	push_back(pp);
}

void Group::DeletePlugin(iterator i, OpenSync::API &api)
{
	// is this plugin connected to a previously saved group config?
	if( (*i)->GetMemberId() != -1 ) {
		// this plugin has a member ID... only OpenSync 0.40 can
		// delete members like that... do we have 40 support?
		OpenSync::OpenSync40 *api40 = dynamic_cast<OpenSync::OpenSync40*> (&api);
		if( !api40 ) {
			// not version 0.40... can't do it capt'n!
			throw DeleteError("Cannot delete individual members from an OpenSync group with versions < 0.40.");
		}

		// so... we have the capability... check that the plugin
		// name of the ID in the group matches what we think it
		// should be

		member_list_type members;
		api40->GetMembers(m_group_name, members);

		Member *m = members.Find( (*i)->GetMemberId() );
		if( !m ) {
			ostringstream oss;
			oss << "Tried to delete non-existent member ID " << (*i)->GetMemberId() << " (" << (*i)->GetPluginName(api) << ") from group '" << m_group_name << "'";
			throw DeleteError(oss.str());
		}

		if( m->plugin_name != (*i)->GetPluginName(api) ) {
			ostringstream oss;
			oss << "Tried to delete member ID " << (*i)->GetMemberId() << " using plugin '" << (*i)->GetPluginName(api) << "' from group '" << m_group_name << "', but the existing member uses plugin '" << m->plugin_name << "'";
			throw DeleteError(oss.str());
		}

		// so far so good... try deleting it
		api40->DeleteMember(m_group_name, (*i)->GetMemberId());
	}

	// remove from our own array
	erase(i);
}

void Group::Save(OpenSync::API &api)
{
	if( GroupExists(api) ) {
		// groups already exists, so need to confirm that our
		// connected plugins match the existing member_ids and
		// plugin names in the group's member list
		if( !GroupMatchesExistingConfig(api) ) {
			ostringstream oss;
			oss << "Tried to overwrite group '" << m_group_name << "' with a Group set that did not match in ID's and plugin names.";
			throw SaveError(oss.str());
		}
	}
	else {
		// group does not exist, so create it if needed
		if( size() )
			api.AddGroup(m_group_name);
	}

	// cycle through all plugins and save them all
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		Config::Plugin &p = *(*b);

		if( p.GetMemberId() == -1 ) {
			// this plugin has never been saved yet, so
			// add it fresh
			long newid = api.AddMember(m_group_name,
				p.GetPluginName(api), "");
			p.SetMemberId(newid);
		}

		// save config
		p.Save(api, m_group_name);
	}
}

bool Group::Compare(const Group &other) const
{
	// size of group equal?
	if( size() != other.size() )
		return false;

	// name of group?
	if( m_group_name != other.m_group_name )
		return false;

	// cycle through all plugins, searching for a match in other
	const_iterator i = begin();
	for( ; i != end(); ++i ) {
		bool sametype, equal;
		bool match = false;

		// search other for a match
		const_iterator oi = other.begin();
		for( ; oi != other.end(); ++oi ) {
			if( (match = (*i)->Compare(*(*oi), sametype, equal)) )
				break;
		}

		if( !match )
			return false;
	}

	return true;
}

Group::group_ptr Group::Clone() const
{
	group_ptr g( new Group(m_group_name) );

	// clone all plugins
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		plugin_ptr p( (*b)->Clone() );
		g->push_back(p);
	}

	return g;
}

}} // namespace OpenSync::Config

