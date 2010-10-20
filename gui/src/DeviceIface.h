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
#include <string>
#include <memory>
#include <stdint.h>
#include "tarfile.h"

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
	Device();
	Device(Barry::ProbeResult);

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
	std::auto_ptr<reuse::TarFile> m_tar, m_tarback;

	// parser and builder data (only one side uses these at a time)
	Barry::ConfigFile::DBListType m_dbList;
	mutable Glib::Mutex *m_dbnameMutex;
	std::string m_current_dbname_not_thread_safe;
	std::string m_current_dbname;
	uint8_t m_rec_type;
	uint32_t m_unique_id;
	std::string m_tar_id_text;
	std::string m_record_data;
	bool m_end_of_tar;
	bool m_tar_record_loaded;

	// thread quit flag... not locked, only a byte
	volatile bool m_thread_quit;

protected:
	bool False(const std::string &msg);

	// threads
	void BackupThread();
	void RestoreThread();
	void RestoreAndBackupThread();

	// helpers
	std::string MakeFilename(const std::string &label = "") const;
	int CountFiles(reuse::TarFile &tar, const Barry::ConfigFile::DBListType &restoreList) const;
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
	// this is for debugging... starts a restore, and then does an
	// immediate backup of the same DB before moving on to the next
	bool StartRestoreAndBackup(AppComm comm,
		const Barry::ConfigFile::DBListType &restoreAndBackupList,
		const std::string &tarfilename,
		const std::string &directory);

	// Barry::Parser overrides
	virtual void StartParser();
	virtual void ParseRecord(const Barry::DBData &data,
		const Barry::IConverter *ic);
	virtual void EndParser();

	// Barry::Builder overrides
	virtual bool Retrieve();
	virtual bool EndOfFile() const { return false; } // not used
	virtual void BuildRecord(Barry::DBData &data, size_t &offset, const Barry::IConverter *ic);
	virtual void BuildDone();
	void SkipCurrentDB() throw();	// helper function for halding restore errors
};

#endif

