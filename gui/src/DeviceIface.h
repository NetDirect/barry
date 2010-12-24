///
/// \file	DeviceIface.h
///		Interface class for device backup and restore
///

/*
    Copyright (C) 2007-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <barry/barrybackup.h>
#include <string>
#include <memory>
#include <stdint.h>

#define DI_THREAD_DONE 100
#define DI_THREAD_PROGRESS 101

namespace Glib {
	class Dispatcher;
	class Mutex;
}

class Device
{
	Barry::ProbeResult result;

public:
	Device(const Barry::ProbeResult &result);

	Barry::Pin GetPIN() const { return Barry::Pin(result.m_pin); };

	friend class DeviceInterface;
};

class DeviceInterface :
	public Barry::Parser,
	public Barry::Builder
{
public:
	struct AppComm				// app communication
	{
		Glib::Dispatcher *m_erase_db;	// to notify the app about the
						// db erase stage of restore
		Glib::Dispatcher *m_restored_db;// to notify the app that the
						// previous erase_db was
						// restored... do not rely
						// on the current db name ==
						// the name found at erase time
		Glib::Dispatcher *m_progress;
		Glib::Dispatcher *m_error;
		Glib::Dispatcher *m_done;

		AppComm() :
			m_erase_db(0),
			m_restored_db(0),
			m_progress(0),
			m_error(0),
			m_done(0)
			{}
		AppComm(Glib::Dispatcher *progress,
			Glib::Dispatcher *error,
			Glib::Dispatcher *done,
			Glib::Dispatcher *erase_db,
			Glib::Dispatcher *restored_db) :
			m_erase_db(erase_db),
			m_restored_db(restored_db),
			m_progress(progress),
			m_error(error),
			m_done(done)
			{}
		bool IsValid() const
			{ return m_erase_db && m_restored_db && m_progress && m_error && m_done; }
		void Invalidate()
			{ m_erase_db = m_restored_db = m_progress = m_error = m_done = 0; }
	};

	class Quit	// quit exception to break out of upload/download
	{
	};

private:
	Device *m_dev;
	Barry::Controller *m_con;
	Barry::Mode::Desktop *m_desktop;
	std::string m_last_error;
	std::string m_last_thread_error;

	AppComm m_AppComm;

	std::auto_ptr<Barry::Backup> m_backup;
	std::auto_ptr<Barry::Restore> m_restore;

	// parser and builder data
	Barry::ConfigFile::DBListType m_dbBackupList;
	mutable Glib::Mutex *m_dbnameMutex;
	std::string m_current_dbname;

	// thread quit flag... not locked, only a byte
	volatile bool m_thread_quit;

protected:
	bool False(const std::string &msg);

	// threads
	void BackupThread();
	void RestoreThread();

	// helpers
	std::string MakeFilename(const std::string &label = "") const;
	bool SplitTarPath(const std::string &tarpath, std::string &dbname,
		std::string &dbid_text, uint8_t &dbrectype, uint32_t &dbid) const;

	// Sets the name of the database the thread is currently working on
	void SetThreadDBName(const std::string &dbname);

public:
	DeviceInterface(Device *dev = 0);
	~DeviceInterface();

	const std::string& get_last_error() const { return m_last_error; }
	const std::string& get_last_thread_error() const { return m_last_thread_error; }

	void Reset();
	bool Connect();
	bool Password(const char *password);
	void Disconnect();

	const Barry::DatabaseDatabase& GetDBDB() const { return m_desktop->GetDBDB(); }
	unsigned int GetRecordTotal(const Barry::ConfigFile::DBListType &backupList) const;
	unsigned int GetRecordTotal(const Barry::ConfigFile::DBListType &restoreList, const std::string &filename) const;

	void QuitThread()	{ m_thread_quit = true; }

	/// returns name of database the thread is currently working on
	std::string GetThreadDBName() const;

	bool StartBackup(AppComm comm,
		const Barry::ConfigFile::DBListType &backupList,
		const std::string &directory, const std::string &backupLabel);
	bool StartRestore(AppComm comm,
		const Barry::ConfigFile::DBListType &restoreList,
		const std::string &tarfilename);

	// Barry::Parser overrides
	virtual void ParseRecord(const Barry::DBData &data,
		const Barry::IConverter *ic);

	// Barry::Builder overrides
	virtual bool BuildRecord(Barry::DBData &data, size_t &offset, const Barry::IConverter *ic);
	virtual bool FetchRecord(Barry::DBData &data, const Barry::IConverter *ic);
	virtual bool EndOfFile() const { return false; } // not used
	void SkipCurrentDB() throw();	// helper function for halding restore errors
};

#endif

