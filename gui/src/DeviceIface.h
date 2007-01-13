///
/// \file	DeviceIface.h
///		Interface class for device backup and restore
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_DEVICEIFACE_H__
#define __BARRYBACKUP_DEVICEIFACE_H__

#include <barry/barry.h>
#include <string>
#include "ConfigFile.h"
#include "tarfile.h"

#define DI_THREAD_DONE 100
#define DI_THREAD_PROGRESS 101

namespace Glib {
	class Dispatcher;
}

class DeviceInterface : public Barry::Parser
{
	Barry::Controller *m_con;
	std::string m_last_error;
	std::string m_last_thread_error;

	Glib::Dispatcher *m_signal_progress, *m_signal_done;
	std::auto_ptr<reuse::TarFile> m_tar;
	ConfigFile::DBListType m_dbList;

	// parser data
	std::string m_current_dbname;
	uint32_t m_unique_id;
	std::string m_unique_id_text;
	std::string m_record_data;

	// thread quit flag... not locked, only a byte
	volatile bool m_thread_quit;

protected:
	bool False(const std::string &msg);

	void BackupThread();
	void RestoreThread();

public:
	DeviceInterface();
	~DeviceInterface();

	const std::string& get_last_error() const { return m_last_error; }
	const std::string& get_last_thread_error() const { return m_last_thread_error; }
	const Barry::DatabaseDatabase& GetDBDB() const { return m_con->GetDBDB(); }

	bool Connect(const Barry::ProbeResult &dev);
	void Disconnect();

	void QuitThread()	{ m_thread_quit = true; }

	bool StartBackup(Glib::Dispatcher *progress, Glib::Dispatcher *done,
		const ConfigFile::DBListType &backupList,
		const std::string &filename);
	bool StartRestore(Glib::Dispatcher *progress, Glib::Dispatcher *done,
		const ConfigFile::DBListType &backupList,
		const std::string &filename);

	// Barry::Parser overrides
	virtual void SetUniqueId(uint32_t Id);
	virtual void ParseFields(const Data &data, size_t &offset);
	virtual void Store();
};

#endif

