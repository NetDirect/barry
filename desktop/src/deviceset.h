///
/// \file	deviceset.h
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_DEVICE_SET_H__
#define __BARRY_DEVICE_SET_H__

#include <barry/barry.h>
#include <memory>
#include <tr1/memory>
#include <stdexcept>

// forward declarations
namespace OpenSync {
	class APISet;
}

namespace OpenSync { namespace Config {

// Exception class for configuration load errors
class LoadError : public std::exception
{
	std::string m_msg;
pubic:
	LoadError(const std::string &msg) : m_msg(msg) {}
	virtual ~LoadError() {}
	virtual const char* what() const throw() { return m_msg.c_str(); }
};

/// Thrown when a member cannot be deleted from the group
class DeleteError : public std::logic_error
{
public:
	DeleteError(const std::string &msg) : std::logic_error(msg) {}
};

class Converter22 : public Converter
{
};
class Converter40 : public Converter
{
};

// Base class for all opensync plugin configurations
class Plugin
{
	long m_member_id;		// -1 if not saved to the group yet

public:
	Plugin();
	virtual ~Plugin();

	long GetMemberId() const { return m_member_id; }
	void SetMemberId(long id) { m_member_id = id; }

	//
	// operations
	//

	/// IsUnsupported() returns true if the config format for this
	/// plugin has no OpenSync::Config::* class to parse / handle it.
	virtual bool IsUnsupported() const { return true; }
	virtual std::string GetAppName() const = 0; // returns a GUI-friendly
				// name for the application that this plugin
				// is for... i.e. plugin name might
				// be "evo2-sync" but the app name
				// would be "Evolution"
	virtual void Save(OpenSync::API &api) const = 0;
	// sample implementation:
	//{
	//	api.GetConverter()->Save(*this);
	//}
	virtual std::string GetPluginName(OpenSync::API &api) const = 0;
	// sample implementation:
	//{
	//	return api.GetConverter()->GetPluginName(*this);
	//}
};

class Barry : public Plugin
{
private:
	// configuration settings
	bool m_debug_mode;
	Barry::Pin m_pin;
	std::string m_password;

protected:
	// This constructor is protected, so that only derived
	// classes can create a configuration without a pin number.
	Barry()
		: m_debug_mode(false)
	{
	}

public:
	explicit Barry(const Barry::Pin &pin)
		: m_debug_mode(false)
		, m_pin(pin)
	{
		if( !m_pin.IsValid() )
			throw std::logic_error("Barry config must have valid pin number.");
	}

	explicit Barry(Converter *load_converter)
		: m_debug_mode(false)
	{
		load_converter->Load(*this);
		//
		// check that the loaded pin is valid... if not, it is
		// likely safe to assume that something is horribly wrong.
		// in the case where the Application wishes to add a new
		// barry plugin, it should use the Pin constructor.
		// if you *really* need to try to salvage and old
		// corrupt config, you can always do the
		// converter->Load(barry_obj) manually, and pick out
		// the left overs.
		//
		if( !m_pin.IsValid() )
			throw std::logic_error("Unable to load pin number from Barry plugin config.  Consider this group corrupt.");
	}

	bool IsDebugMode() const { return m_debug_mode; }
	Barry::Pin GetPin() const { return m_pin; }
	std::string GetPassword() const { return m_password; }

	void DebugMode(bool mode = true) { m_debug_mode = mode; }
	void SetPin(const Barry::Pin &pin) { m_pin = pin; }
	void SetPassword(const std::string &pass) { m_password = pass; }

	// virtual overrides
	virtual bool IsUnsupported() const { return false; }
	virtual std::string GetAppName() const { return AppName(); }
	virtual void Save(OpenSync::API &api) const
	{
		api.GetConverter()->Save(*this);
	}
	virtual std::string GetPluginName(OpenSync::API &api) const
	{
		return api.GetConverter()->GetPluginName(*this);
	}

	// statics
	std::string AppName() { return "Barry"; }
};

//
// Group
//
/// This class handles the loading of all the config plugins in a
/// given group.
///
class Group :
	private std::vector<std::tr1::shared_ptr<OpenSync::Config::Plugin> > >
{
public:
	typedef std::tr1::shared_ptr<OpenSync::Config::Plugin>	value_type;
	typedef std::vector<value_type>				base_type;
	typedef base_type::iterator				iterator;
	typedef base_type::const_iterator			const_iterator;

private:
	std::string m_group_name;

protected:

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
	///		for which there is no corresponding Plugin
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

	/// Remove the plugin from the group.  Only can remove non-saved
	/// plugins.  If a plugin has already been saved to the underlying
	/// OpenSync API, it may not be possible to delete just one
	/// member from the group (0.22), and a DeleteError will be thrown.
	void DeletePlugin(iterator i);

	/// Save all plugins as members of the group.  If the group doesn't
	/// exist, it will be created.  If a plugin doesn't have a member_id,
	/// it will be added as a new member to the group and given an id.
	/// Note that an exception thrown from this function means that
	/// the group is left in an undefined state.  Best to delete it
	/// and start over.  Or load it, delete it (through the API), and
	/// save it again.
	void Save(OpenSync::API &api);

	/// Forget all plugins and delete them all
	using clear;

	// bring underlying implementation forward
	using begin;
	using end;
	using operator[];
};


}} // namespace OpenSync::Config

//
// DeviceEntry
//
/// Entry in the DeviceSet class.  This class must be STL container safe.
///
class DeviceEntry
{
private:
	// pointers to external data
	// pointers may be 0 if data is not available (such as when
	// a device is not connected, it would not have a ProbeResult)
	const Barry::ProbeResult *m_result;	// pointer to external data

	std::tr1::shared_ptr<OpenSync::Config::Group> m_group;

protected:
	OpenSync::Config::Barry* FindBarry();	// returns pointer to the Barry
						// plugin object in m_group
						// or 0 if not available

public:
	DeviceEntry(const Barry::ProbeResult *result,
		std::tr1::shared_ptr<OpenSync::Config::Group> group);

	Barry::Pin GetPin() const;
	std::string GetDeviceName() const;
	bool IsConnected() const;
	bool IsConfigured() const;

	const Barry::ProbeResult* GetProbeResult() { return m_result; }
	OpenSync::Config::Barry* GetConfig() { return m_config.get(); }
};

//
// DeviceSet
//
/// This class detects known devices on an opensync-able system.
/// It will search for connected (USB) devices and devices that have been
/// configured in OpenSync (both 0.22 and 0.4x) but are not currently
/// connected.
///
/// For each device entry, it will know the following:
///	- pin
///	- device name (from barrybackup configs)
///	- whether connected or not
///	- whether configured for opensync or not... if so, it will also
///		keep track of:
///		- the app(s) it will sync with (in one sync group only)
///		- the version of the engine it is configured with
///
/// If a device is configured in both 0.4x and 0.22, then the first one
/// found is used.
///
/// Since this class needs to open and parse a lot of information during
/// construction anyway, it will store as much as possible, to allow for
/// editing in a GUI and saving it later.  The domain specific data
/// may be encapsulated in further classes.
///
class DeviceSet : public std::vector<DeviceEntry>
{
public:
	typedef std::vector<DeviceEntry>		base_type;
	typedef base_type::iterator			iterator;
	typedef base_type::const_iterator		const_iterator;

private:
	OpenSync::APISet &m_apiset;
	Barry::Probe::Results m_results;

public:
	/// Does a USB probe automatically
	explicit DeviceSet(OpenSync::APISet &apiset);

	/// Skips the USB probe and uses the results set given
	DeviceSet(const Barry::Probe::Results &results, OpenSync::APISet &apiset);
};

#endif

