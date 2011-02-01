///
/// \file	Mode_Browse.h
///		Mode derived class for database browsing
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_MODE_BROWSE_H__
#define __BARRYDESKTOP_MODE_BROWSE_H__

#include "Mode.h"
#include "util.h"
#include <barry/barry.h>
#include <barry/scoped_lock.h>
#include <tr1/memory>
#include <list>

// Holds the "right" to use the desktop, and this right expires with the
// scope of this class, and no other code can use it while this object exists.
//
// Is copied around by the below auto_ptr<>
class DesktopInstance
{
private:
	Barry::scoped_lock m_lock;

	// external interfaces... owned by caller
	Barry::Mode::Desktop &m_desktop;
	Barry::IConverter &m_ic;

public:
	DesktopInstance(pthread_mutex_t &mutex, Barry::Mode::Desktop &desktop,
		Barry::IConverter &ic)
		: m_lock(mutex)
		, m_desktop(desktop)
		, m_ic(ic)
	{
	}

	Barry::Mode::Desktop& Desktop() { return m_desktop; }
	Barry::IConverter& IC() { return m_ic; }
};

typedef std::auto_ptr<DesktopInstance> DesktopInstancePtr;

class ThreadableDesktop
{
private:
	pthread_mutex_t m_mutex;

	// external interfaces... owned by caller
	Barry::Mode::Desktop &m_desktop;
	Barry::IConverter &m_ic;

public:
	ThreadableDesktop(Barry::Mode::Desktop &desktop, Barry::IConverter &ic)
		: m_desktop(desktop)
		, m_ic(ic)
	{
		if( pthread_mutex_init(&m_mutex, NULL) ) {
			throw Barry::Error("Failed to create mutex for ThreadableDesktop");
		}
	}

	DesktopInstancePtr Get()
	{
		return DesktopInstancePtr(
			new DesktopInstance(m_mutex, m_desktop, m_ic));
	}
};

class DataCache
{
public:
	typedef Barry::RecordStateTable::IndexType IndexType;

private:
	IndexType m_state_index;

public:
	explicit DataCache(IndexType state_index)
		: m_state_index(state_index)
	{
	}

	virtual ~DataCache() {}

	IndexType GetStateIndex() const { return m_state_index; }

	// set editable to false if you just want to view record
	// non-buildable records will always be non-editable regardless
	virtual void Edit(wxWindow *parent, bool editable) = 0;
	virtual std::string GetDescription() const = 0;

	virtual bool IsBuildable() const { return false; }
	virtual void Build(Barry::DBData &data, size_t offset,
		const Barry::IConverter *ic) const
	{
		// not buildable
	}

	bool operator< (const DataCache &other)
	{
		return GetDescription() < other.GetDescription();
	}
};

typedef std::tr1::shared_ptr<DataCache> DataCachePtr;

class DBDataCache : public DataCache
{
	Barry::DBData m_raw;

public:
	DBDataCache(DataCache::IndexType index, const Barry::DBData &raw);

	virtual void Edit(wxWindow *parent, bool editable);
	virtual std::string GetDescription() const;
};

// returns true if GUI indicates change (i.e. if dialogreturns wxID_OK)
#undef HANDLE_PARSER
#define HANDLE_PARSER(dbname) \
	bool EditRecord(wxWindow *parent, bool editable, Barry::dbname &rec);
ALL_KNOWN_PARSER_TYPES

template <class RecordT>
class RecordCache : public DataCache
{
private:
	RecordT m_rec;

public:
	RecordCache(DataCache::IndexType index, const RecordT &rec)
		: DataCache(index)
		, m_rec(rec)
	{
	}

	virtual void Edit(wxWindow *parent, bool editable)
	{
		RecordT copy = m_rec;
		bool changed = EditRecord(parent, editable, copy);
		if( changed && editable ) {
			m_rec = copy;
		}
	}

	virtual std::string GetDescription() const
	{
		return m_rec.GetDescription();
	}

	virtual bool IsBuildable() const
	{
		return ::IsBuildable<RecordT>();
	}

	virtual void Build(Barry::DBData &data, size_t offset,
		const Barry::IConverter *ic)
	{
// FIXME
//		Barry::SetDBData(m_rec, data, 0, ic);
	}
};

class DBCache : public Barry::AllRecordStore, public Barry::Parser
{
public:
	typedef Barry::RecordStateTable::IndexType	IndexType;
	typedef std::list<DataCachePtr>			DataList;
	typedef DataList::iterator			iterator;
	typedef DataList::const_iterator		const_iterator;

private:
	ThreadableDesktop &m_tdesktop;
	std::string m_dbname;
	unsigned int m_dbid;
	Barry::RecordStateTable m_state;
	DataList m_records;

	// per-record load state
	IndexType m_index;

public:
	// loads records in constructor
	DBCache(ThreadableDesktop &tdesktop, const std::string &dbname);
	virtual ~DBCache();

	const std::string& GetDBName() { return m_dbname; }

//	virtual void Add(wxWindow *parent);
//	virtual void Edit(wxWindow *parent, int list_offset);
//	virtual void Delete(wxWindow *parent, int list_offset);

	DataList::const_iterator begin() const { return m_records.begin(); }
	DataList::const_iterator end() const { return m_records.end(); }

	// For Barry::AllRecordStore
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
        virtual void operator() (const Barry::tname &);
	ALL_KNOWN_PARSER_TYPES

	// For Barry::Parser
	virtual void ParseRecord(const Barry::DBData &data,
		const Barry::IConverter *ic);
};

class DBMap
{
public:
	typedef std::tr1::shared_ptr<DBCache>		DBCachePtr;
	typedef std::map<std::string, DBCachePtr>	MapType;

private:
	ThreadableDesktop &m_tdesktop;
	MapType m_map;
	pthread_mutex_t m_map_mutex;

public:
	DBMap(ThreadableDesktop &tdesktop);

	DBCachePtr GetDBCache(const std::string &dbname);
};

class BrowseMode : public wxEvtHandler, public Mode
{
private:
	DECLARE_EVENT_TABLE()

private:
	wxWindow *m_parent;

	// device interface
	std::auto_ptr<Barry::IConverter> m_ic;
	std::auto_ptr<Barry::Controller> m_con;
	std::auto_ptr<Barry::Mode::Desktop> m_desktop;
	std::auto_ptr<ThreadableDesktop> m_tdesktop;
	std::auto_ptr<DBMap> m_dbmap;
	Barry::DatabaseDatabase m_dbdb;

	// window controls
	std::auto_ptr<wxBoxSizer> m_top_sizer;
	std::auto_ptr<wxListCtrl> m_dbdb_list;
	std::auto_ptr<wxListCtrl> m_record_list;
	std::auto_ptr<wxCheckBox> m_show_all_checkbox;
	std::auto_ptr<wxButton> m_add_record_button;
	std::auto_ptr<wxButton> m_edit_record_button;
	std::auto_ptr<wxButton> m_delete_record_button;

	// misc supporting data
	wxString m_device_id_str;

	// GUI state
	bool m_buildable;		// true if currently displayed db has
					// a Builder available for it
	bool m_show_all;		// if true, show all databases in list
					// instead of just the parsable ones

	// thread state
	pthread_t m_cache_thread;
	volatile bool m_abort_flag;

protected:
	void CreateControls();
	int GUItoDBDBIndex(int gui_index);
	void FillDBDBList();
	void FillRecordList(const std::string &dbname);
	void UpdateButtons();
	void FillCache();

	// background thread function
	static void* FillCacheThread(void *bobj);

public:
	BrowseMode(wxWindow *parent, const Barry::ProbeResult &device);
	~BrowseMode();

	// virtual override events (derived from Mode)
	wxString GetTitleText() const { return _T("Barry Database Browser"); }

	// window events
	void OnDBDBListSelChange(wxListEvent &event);
	void OnShowAll(wxCommandEvent &event);
	void OnAddRecord(wxCommandEvent &event);
	void OnEditRecord(wxCommandEvent &event);
	void OnDeleteRecord(wxCommandEvent &event);
};

#endif

