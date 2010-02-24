///
/// \file	osconfig.h
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_OSCONFIG_H__
#define __BARRY_OSCONFIG_H__

#include <barry/barry.h>
#include <memory>
#include <tr1/memory>
#include <stdexcept>
#include "osbase.h"
#include <iostream>

namespace OpenSync { namespace Config {

// Exception class for configuration load errors
class LoadError : public std::exception
{
	std::string m_msg;
public:
	LoadError(const std::string &msg) : m_msg(msg) {}
	virtual ~LoadError() throw() {}
	virtual const char* what() const throw() { return m_msg.c_str(); }
};

class SaveError : public std::exception
{
	std::string m_msg;
public:
	SaveError(const std::string &msg) : m_msg(msg) {}
	virtual ~SaveError() throw() {}
	virtual const char* what() const throw() { return m_msg.c_str(); }
};

/// Thrown when a member cannot be deleted from the group
class DeleteError : public std::logic_error
{
public:
	DeleteError(const std::string &msg) : std::logic_error(msg) {}
};

// Base class for all opensync plugin configurations
class Plugin
{
	long m_member_id;		// -1 if not saved to the group yet

public:
	Plugin()
		: m_member_id(-1)
	{
	}

	virtual ~Plugin()
	{
	}

	long GetMemberId() const { return m_member_id; }
	void SetMemberId(long id) { m_member_id = id; }

	//
	// operations
	//

	virtual Plugin* Clone() const = 0;

	/// IsUnsupported() returns true if the config format for this
	/// plugin has no OpenSync::Config::* class to parse / handle it.
	virtual bool IsUnsupported() const { return true; }
	virtual std::string GetAppName() const = 0; // returns a GUI-friendly
				// name for the application that this plugin
				// is for... i.e. plugin name might
				// be "evo2-sync" but the app name
				// would be "Evolution"
	/// Throws SaveError if member_id is -1
	virtual void Save(OpenSync::API &api, const std::string &group_name) const = 0;
	// sample implementation:
	//{
	//	api.GetConverter().Save(*this, group_name);
	//}
	virtual std::string GetPluginName(OpenSync::API &api) const = 0;
	// sample implementation:
	//{
	//	return api.GetConverter().GetPluginName(*this);
	//}
};

class Unsupported : public Plugin
{
private:
	// configuration settings
	std::string m_raw_config;

public:
	Unsupported()
	{
	}

	explicit Unsupported(Converter *load_converter, const Member &member)
	{
		load_converter->Load(*this, member);
	}

	const std::string& GetRawConfig() const { return m_raw_config; }

	void SetRawConfig(const std::string &c) { m_raw_config = c; }

	// virtual overrides
	virtual Unsupported* Clone() const { return new Unsupported(*this); }
	virtual bool IsUnsupported() const { return true; }
	virtual std::string GetAppName() const { return AppName(); }
	virtual void Save(OpenSync::API &api, const std::string &group_name) const
	{
		api.GetConverter().Save(*this, group_name);
	}
	virtual std::string GetPluginName(OpenSync::API &api) const
	{
		return api.GetConverter().GetPluginName(*this);
	}

	// statics
	static std::string AppName() { return "Unsupported"; }
};

class Barry : public Plugin
{
private:
	// configuration settings
	bool m_debug_mode;
	::Barry::Pin m_pin;
	std::string m_password;

protected:
	// This constructor is protected, so that only derived
	// classes can create a configuration without a pin number.
	Barry()
		: m_debug_mode(false)
	{
	}

public:
	explicit Barry(const ::Barry::Pin &pin)
		: m_debug_mode(false)
		, m_pin(pin)
	{
		if( !m_pin.valid() )
			throw std::logic_error("Barry config must have valid pin number.");
	}

	explicit Barry(Converter *load_converter, const Member &member)
		: m_debug_mode(false)
	{
		load_converter->Load(*this, member);
		//
		// check that the loaded pin is valid... if not, it is
		// likely safe to assume that something is horribly wrong.
		// in the case where the Application wishes to add a new
		// barry plugin, it should use the Pin constructor.
		// if you *really* need to try to salvage an old
		// corrupt config, you can always do the
		// converter->Load(barry_obj) manually, and pick out
		// the left overs.
		//
		if( !m_pin.valid() ) {
			std::ostringstream oss;
			oss << "Unable to load pin number from Barry plugin config.  Consider this group corrupt, or not fully configured: " << member;
			throw LoadError(oss.str());
		}
	}

	bool IsDebugMode() const { return m_debug_mode; }
	::Barry::Pin GetPin() const { return m_pin; }
	std::string GetPassword() const { return m_password; }

	void DebugMode(bool mode = true) { m_debug_mode = mode; }
	void SetPin(const ::Barry::Pin &pin) { m_pin = pin; }
	void SetPassword(const std::string &pass) { m_password = pass; }

	// virtual overrides
	virtual Barry* Clone() const { return new Barry(*this); }
	virtual bool IsUnsupported() const { return false; }
	virtual std::string GetAppName() const { return AppName(); }
	virtual void Save(OpenSync::API &api, const std::string &group_name) const
	{
		api.GetConverter().Save(*this, group_name);
	}
	virtual std::string GetPluginName(OpenSync::API &api) const
	{
		return api.GetConverter().GetPluginName(*this);
	}

	// statics
	static std::string AppName() { return "Barry"; }
	static std::string PluginName(OpenSync::API &api)
	{
		return api.GetConverter().GetPluginName(Barry());
	}
};

class Evolution : public Plugin
{
private:
	// configuration settings
	std::string m_address_path;
	std::string m_calendar_path;
	std::string m_tasks_path;
	std::string m_memos_path;

protected:
	static void SetIfExists(std::string &var, const std::string &dir);

public:
	Evolution()
	{
	}

	explicit Evolution(Converter *load_converter, const Member &member)
	{
		load_converter->Load(*this, member);
	}

	const std::string& GetAddressPath() const { return m_address_path; }
	const std::string& GetCalendarPath() const { return m_calendar_path; }
	const std::string& GetTasksPath() const { return m_tasks_path; }
	const std::string& GetMemosPath() const { return m_memos_path; }

	void SetAddressPath(const std::string &p) { m_address_path = p; }
	void SetCalendarPath(const std::string &p) { m_calendar_path = p; }
	void SetTasksPath(const std::string &p) { m_tasks_path = p; }
	void SetMemosPath(const std::string &p) { m_memos_path = p; }

	// specific operations
	bool AutoDetect();	// throw if unable to detect??

	// virtual overrides
	virtual Evolution* Clone() const { return new Evolution(*this); }
	virtual bool IsUnsupported() const { return false; }
	virtual std::string GetAppName() const { return AppName(); }
	virtual void Save(OpenSync::API &api, const std::string &group_name) const
	{
		api.GetConverter().Save(*this, group_name);
	}
	virtual std::string GetPluginName(OpenSync::API &api) const
	{
		return api.GetConverter().GetPluginName(*this);
	}

	// statics
	static std::string AppName() { return "Evolution"; }
	static std::string PluginName(OpenSync::API &api)
	{
		return api.GetConverter().GetPluginName(Evolution());
	}
};

//
// Group
//
/// This class handles the loading of all the config plugins in a
/// given group.
///
class Group :
	private std::vector<std::tr1::shared_ptr<OpenSync::Config::Plugin> >
{
public:
	typedef std::tr1::shared_ptr<OpenSync::Config::Plugin>	value_type;
	typedef value_type					plugin_ptr;
	typedef std::vector<value_type>				base_type;
	typedef base_type::iterator				iterator;
	typedef base_type::const_iterator			const_iterator;

private:
	std::string m_group_name;

protected:
	static void BarryCheck(OpenSync::API &api,
		const std::string &group_name,
		const member_list_type &members,
		unsigned throw_mask);
	bool GroupMatchesExistingConfig(OpenSync::API &api);

public:
// OpenSync Config Group Throw Masks
#define OSCG_THROW_ON_UNSUPPORTED	0x01
#define OSCG_THROW_ON_NO_BARRY		0x02
#define OSCG_THROW_ON_MULTIPLE_BARRIES	0x04

	/// This constructor loads an existing Group from the
	/// given OpenSync api resource, filling the vector
	/// array with the required plugin config classes.
	///
	/// If desired, specify a mask, which will cause the
	/// constructor to throw exceptions on the given conditions.
	///
	/// OSCG_THROW_ON_UNSUPPORTED - if set, the constructor will
	///		throw LoadError if there is a plugin in the group
	///		for which there is no corresponding Plugin-
	///		derived class to handle it
	/// OSCG_THROW_ON_NO_BARRY - if set, the constructor will
	///		throw LoadError if there are no Barry plugins
	///		in the group
	/// OSCG_THROW_ON_MULTIPLE_BARRIES - if set, the constructor will
	///		throw LoadError if there is more than one Barry
	///		plugin in the group
	///
	Group(const std::string &group_name, OpenSync::API &api,
		unsigned throw_mask = 0);

	/// This constructor creates an empty, but named, Group.
	explicit Group(const std::string &group_name);

	bool HasUnsupportedPlugins() const;
	bool HasBarryPlugins() const;
	bool GroupExists(OpenSync::API &api) const;
	int GetConnectedCount() const;
	const std::string& GetGroupName() const { return m_group_name; }

	/// Returns a reference to the (first) Barry plugin in the group.
	/// Will throw std::logic_error if not found.
	OpenSync::Config::Barry& GetBarryPlugin();
	const OpenSync::Config::Barry& GetBarryPlugin() const;

	/// Returns a pointer to the first non-Barry plugin in the group.
	/// Returns 0 if not found.
	OpenSync::Config::Plugin* GetNonBarryPlugin();
	const OpenSync::Config::Plugin* GetNonBarryPlugin() const;

	/// Sets the member_id of all plugins to -1, thereby making them
	/// appear like they have never been saved to an API before.
	/// A call to Save() after a call to DisconnectMembers() will
	/// create a completely new group.  Note that if the group name
	/// already exists in the API, saving to it again may cause
	/// a low level exception to be thrown.
	void DisconnectMembers();

	/// Loads all available plugins from the named group, using
	/// the api given.  If this Group isn't empty (size() != 0),
	/// then DisconnectMembers() will be called first.
	void Load(OpenSync::API &api, unsigned throw_mask = 0);
	/// Same as Load() above, but loads from src_group_name instead
	/// of m_group_name.  m_group_name will not be changed.
	void Load(const std::string &src_group_name, OpenSync::API &api,
		unsigned throw_mask = 0);

	/// Overwrites m_group_name with a new one.
	void ResetGroupName(const std::string &new_group_name);

	/// Adds plugin to group, and takes ownership of the pointer
	void AddPlugin(OpenSync::Config::Plugin *plugin);

	/// Remove the plugin from the group.  Only can guarantee removal
	/// of non-saved (i.e. disconnected) plugins.  If a plugin has
	/// already been saved to the underlying OpenSync API, it
	/// may not be possible to delete just one member from the
	/// group (0.22), and a DeleteError will be thrown.
	///
	/// If you don't want to actually delete any members from any
	/// existing configs, then DisconnectMembers() first.
	void DeletePlugin(iterator i, OpenSync::API &api);

	/// Save all plugins as members of the group.  If the group doesn't
	/// exist, it will be created.  If the group does exist, and its
	/// existing plugin list and member_id's don't match this Group's
	/// list, SaveError will be thrown.  If a plugin doesn't have a
	/// member_id, it will be added as a new member to the group and
	/// given an id.  Note that an exception thrown from this function
	/// means that the group is left in an undefined state.  Best to
	/// delete it and start over.  Or load it, delete it (through the API),
	/// disconnect it, and save it again.
	void Save(OpenSync::API &api);

	/// Forget all plugins and delete them all
	using base_type::clear;

	// bring underlying implementation forward
	using base_type::size;
	using base_type::begin;
	using base_type::end;
	using base_type::operator[];
};


}} // namespace OpenSync::Config

#endif

