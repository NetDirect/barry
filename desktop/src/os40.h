///
/// \file	os40.h
///		Wrapper class for opensync 0.4x syncing behaviour
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

#ifndef __BARRYDESKTOP_OS40_H__
#define __BARRYDESKTOP_OS40_H__

#include "dlopen.h"
#include "osbase.h"
#include <memory>

namespace OpenSync {

class OpenSync40Private;
class OS40PluginConfigPrivate;
class OS40ConfigResourcePrivate;

class OS40PluginConfig;
class OpenSync40;

class OS40ConfigResource
{
	friend class OS40PluginConfig;

private:
	OS40ConfigResourcePrivate *m_priv;
	bool m_exists;			// true if this resources exists
					// in the plugin config.
					// AddResource() sets this to true

private:
	OS40ConfigResource(const OS40PluginConfig &parent, void *resource,
		bool existing_resource);
	OS40ConfigResource(const OS40ConfigResource &other);//disabled copy
	OS40ConfigResource& operator=(const OS40ConfigResource &other);

public:
	~OS40ConfigResource();

	/// Returns true if this instance represents an existing
	/// resource in the config.  If this is false, then
	/// AddResource() must be called, otherwise any changes
	/// will not be saved to the config object.
	bool IsExistingResource() const;
	void AddResource();	// safe to call multiple times

	bool IsEnabled() const;
	OS40ConfigResource& Enable(bool enabled = true);

	// searches for the objformat, and fills in config with its config
	// value if it exists and returns true... otherwise returns false
	bool FindObjFormat(const std::string &objformat, std::string &config);
	OS40ConfigResource& SetObjFormat(const std::string &objformat,
		const std::string &config = "");

	std::string GetName() const;
	OS40ConfigResource& SetName(const std::string &name);

	std::string GetPreferredFormat() const;
	OS40ConfigResource& SetPreferredFormat(const std::string &format);

	std::string GetMime() const;
	OS40ConfigResource& SetMime(const std::string &mime);

	std::string GetObjType() const;
private:OS40ConfigResource& SetObjType(const std::string &objtype); // objtype is set
		// automatically when this object is created with GetResource()

public:
	std::string GetPath() const;
	OS40ConfigResource& SetPath(const std::string &path);

	std::string GetUrl() const;
	OS40ConfigResource& SetUrl(const std::string &url);
};

class OS40PluginConfig
{
	friend class OS40ConfigResource;
	friend class OpenSync40;

public:
	typedef std::auto_ptr<OS40ConfigResource>	OS40ConfigResourcePtr;

	enum {
		NONE_TYPE,
		BOOL_TYPE,
		CHAR_TYPE,
		DOUBLE_TYPE,
		INT_TYPE,
		LONG_TYPE,
		LONGLONG_TYPE,
		UINT_TYPE,
		ULONG_TYPE,
		ULONGLONG_TYPE,
		STRING_TYPE
	};

private:
	OpenSync40Private *m_privapi;	// external pointer to OpenSync40
	std::tr1::shared_ptr<OS40PluginConfigPrivate> m_priv;

private:
	OS40PluginConfig(OpenSync40Private *privapi, void *member, void *config);

public:
	std::string GetAdvanced(const std::string &name);
	void SetAdvanced(const std::string &name,
		const std::string &display_name, const std::string &val);
	void SetAdvanced(const std::string &name,
		const std::string &display_name,
		int val_type, const std::string &val);

	OS40ConfigResourcePtr GetResource(const std::string &objtype);

	void Save();
};

class OpenSync40 : public DlOpen, public OpenSync::API
{
public:

private:
	// private opensync 0.40 function pointers and data
	OpenSync40Private *m_priv;

protected:
	void SetupEnvironment(OpenSync40Private *p);

public:
	OpenSync40();
	~OpenSync40();

	//
	// Virtual API overrides
	//

	// Functional abilities information... this does not come from
	// the engine itself, but is information the osbase library
	// determines useful for applications to know
	bool IsSlowSyncSupported() const { return true; } // FIXME - is this right?
	bool IsContactSyncSupported() const { return true; }
	bool IsCalendarSyncSupported() const { return true; }
	bool IsMemoSyncSupported() const { return true; }
	bool IsTodoSyncSupported() const { return true; }

	// General engine information
	const char* GetVersion() const;
	const char* GetEngineName() const;
	void GetPluginNames(string_list_type &plugins);
	void GetFormats(format_list_type &formats);

	// Information about configured groups
	void GetGroupNames(string_list_type &groups);
	void GetMembers(const std::string &group_name,
		member_list_type &members);

	// Group configuration
	void AddGroup(const std::string &group_name);
	void DeleteGroup(const std::string &group_name);

	// Plugin configuration helper
	Converter& GetConverter();

	// Member configuration
	long AddMember(const std::string &group_name,
		const std::string &plugin_name,
		const std::string &member_name);
	void DeleteMember(const std::string &group_name, long member_id);
	void DeleteMember(const std::string &group_name,
		const std::string &plugin_name);
	bool IsConfigurable(const std::string &group_name,
		long member_id);
	std::string GetConfiguration(const std::string &group_name,
		long member_id);
	OS40PluginConfig GetConfigurationObj(const std::string &group_name,
		long member_id);
	void SetConfiguration(const std::string &group_name,
		long member_id, const std::string &config_data);
	void Discover(const std::string &group_name);

	// Syncing
	void Sync(const std::string &group_name, SyncStatus &status_callback);
};

} // namespace OpenSync

#endif

