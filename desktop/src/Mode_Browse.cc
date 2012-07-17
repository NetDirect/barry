///
/// \file	Mode_Browse.cc
///		Mode derived class for database browsing
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "Mode_Browse.h"
#include "BaseFrame.h"
#include "ContactEditDlg.h"
#include "CalendarEditDlg.h"
#include "MemoEditDlg.h"
#include "TaskEditDlg.h"
#include "MimeExportDlg.h"
#include "windowids.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace Barry;

DEFINE_EVENT_TYPE(BMET_LOAD_STATUS)

BEGIN_EVENT_TABLE(BrowseMode, wxEvtHandler)
	EVT_LIST_ITEM_SELECTED(BrowseMode_DBDBList,
				BrowseMode::OnDBDBListSelChange)
	EVT_LIST_ITEM_SELECTED(BrowseMode_RecordList,
				BrowseMode::OnRecordListSelChange)
	EVT_LIST_ITEM_ACTIVATED(BrowseMode_RecordList,
				BrowseMode::OnRecordListActivated)
	EVT_CHECKBOX	(BrowseMode_ShowAllCheckbox,
				BrowseMode::OnShowAll)
	EVT_BUTTON	(BrowseMode_ImportRecordButton,
				BrowseMode::OnImportRecord)
	EVT_BUTTON	(BrowseMode_ExportRecordButton,
				BrowseMode::OnExportRecord)
	EVT_BUTTON	(BrowseMode_AddRecordButton,
				BrowseMode::OnAddRecord)
	EVT_BUTTON	(BrowseMode_CopyRecordButton,
				BrowseMode::OnCopyRecord)
	EVT_BUTTON	(BrowseMode_EditRecordButton,
				BrowseMode::OnEditRecord)
	EVT_BUTTON	(BrowseMode_DeleteRecordButton,
				BrowseMode::OnDeleteRecord)
	EVT_COMMAND	(wxID_ANY, BMET_LOAD_STATUS,
				BrowseMode::OnStatusEvent)
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////////////
// Standalone functions

void ShowReadOnlyMsg(wxWindow *parent, const Barry::ReturnCodeError &rce)
{
	wxString what(rce.what(), wxConvUTF8);
	wxMessageBox(_W("This database is apparently read-only.  If this device is connected to a BES, you cannot edit records via USB.  (Error: ") + what + _T(")"),
		_W("Device Error"), wxOK | wxICON_ERROR, parent);
}

bool IsEditable(const std::string &dbname)
{
	// add entry here for each edit dialog available
	return	dbname == Contact::GetDBName() ||
		dbname == Calendar::GetDBName() ||
		dbname == Memo::GetDBName() ||
		dbname == Task::GetDBName();
}

bool IsCardable(const std::string &dbname, std::string *file_types = 0);
bool IsCardable(const std::string &dbname, std::string *file_types)
{
	// add entry here for each record that can handle MIME style
	// import / exports
	if( dbname == Contact::GetDBName() ) {

		if( file_types ) {
			*file_types = _C("VCard files");
			*file_types += " (*.vcf;*.vcard)|*.vcf;*.vcard";
		}
		return true;

	}
	else if( dbname == Calendar::GetDBName() ||
			dbname == Memo::GetDBName() ||
			dbname == Task::GetDBName() ) {

		if( file_types ) {
			*file_types = _C("ICalendar files");
			*file_types += " (*.ical;*.ics;*.ifb;*.icalendar)|*.ical;*.ics;*.ifb;*.icalendar";
		}
		return true;

	}

	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Contact &rec)
{
	ContactEditDlg edit(parent, rec, editable);
	return edit.ShowModal() == wxID_OK;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Bookmark &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Calendar &rec)
{
	CalendarEditDlg edit(parent, rec, editable, &zones);
	return edit.ShowModal() == wxID_OK;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::CalendarAll &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::ContentStore &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Folder &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Memo &rec)
{
	MemoEditDlg edit(parent, rec, editable);
	return edit.ShowModal() == wxID_OK;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Message &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::CallLog &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::PINMessage &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::SavedMessage &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::ServiceBook &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Sms &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::Task &rec)
{
	TaskEditDlg edit(parent, rec, editable, &zones);
	return edit.ShowModal() == wxID_OK;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::TimeZone &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent,
		bool editable,
		const Barry::TimeZones &zones,
		Barry::HandheldAgent &rec)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////////
// GUIDesktopConnector

bool GUIDesktopConnector::PasswordPrompt(const Barry::BadPassword &bp,
					std::string &password_result)
{
	// create prompt based on exception data
	ostringstream oss;
	oss << _C("Please enter device password: ")
	    << "(" << bp.remaining_tries() << _C(" tries remaining") << ")";
	wxString prompt(oss.str().c_str(), wxConvUTF8);

	// ask user for device password
	wxString pass = wxGetPasswordFromUser(prompt,
		_W("Device Password"), _T(""), m_parent);

	password_result = pass.utf8_str();

	// assume that a blank password means the user wishes to quit...
	// wxWidgets doesn't seem to handle this very well?
	return password_result.size() > 0;
}

//////////////////////////////////////////////////////////////////////////////
// DBDataCache

DBDataCache::DBDataCache(DataCache::IndexType index, const Barry::DBData &raw)
	: DataCache(index, raw.GetUniqueId())
	, m_raw(raw)
{
}

bool DBDataCache::Edit(wxWindow *parent,
			bool editable,
			const Barry::TimeZones &zones)
{
	return false;
}

std::string DBDataCache::GetDescription() const
{
	return _C("raw data");
}


//////////////////////////////////////////////////////////////////////////////
// DBCache

DBCache::DBCache(ThreadableDesktop &tdesktop, const std::string &dbname)
	: m_tdesktop(tdesktop)
	, m_dbname(dbname)
{
	// lock the desktop
	DesktopInstancePtr dip = m_tdesktop.Get();

	// grab the DBID
	m_dbid = dip->Desktop().GetDBID(m_dbname);

	// load the record state table
	dip->Desktop().GetRecordStateTable(m_dbid, m_state);

	// load all records
	AllRecordParser ap(*this, *this);
	RecordStateTable::StateMapType::iterator i = m_state.StateMap.begin();
	for( ; i != m_state.StateMap.end(); ++i ) {
		m_index = i->second.Index; // save for the callback
		dip->Desktop().GetRecord(m_dbid, m_index, ap);
	}

	// sort the list of records by description
	m_records.sort();
}

DBCache::~DBCache()
{
}

DBCache::iterator DBCache::Get(int list_offset)
{
	iterator i = begin();
	for( ; i != end() && list_offset; ++i, list_offset-- )
		;
	return i;
}

DBCache::const_iterator DBCache::Get(int list_offset) const
{
	const_iterator i = begin();
	for( ; i != end() && list_offset; ++i, list_offset-- )
		;
	return i;
}

int DBCache::GetIndex(iterator record) const
{
	iterator i = const_cast<DBCache*> (this)->begin();
	iterator e = const_cast<DBCache*> (this)->end();
	for( int index = 0; i != e; ++i, index++ ) {
		if( i == record )
			return index;
	}
	return -1;
}

int DBCache::GetIndex(const_iterator record) const
{
	const_iterator i = begin();
	for( int index = 0; i != end(); ++i, index++ ) {
		if( i == record )
			return index;
	}
	return -1;
}

DBCache::iterator DBCache::Add(wxWindow *parent, DataCachePtr p)
{
	// see if this record has a builder
	Barry::Builder *bp = dynamic_cast<Barry::Builder*> (p.get());
	if( !bp ) {
		cerr << _C("DataCachePtr has no builder") << endl;
		return end();
	}

	// give record a new UniqueID
	uint32_t record_id = m_state.MakeNewRecordId();
cout << "New recordID generated: 0x" << hex << record_id << endl;
	p->SetIds(p->GetStateIndex(), record_id);

	// add record to device
	DesktopInstancePtr dip = m_tdesktop.Get();
	Barry::Mode::Desktop &desktop = dip->Desktop();
bool iv = Barry::IsVerbose();
Barry::Verbose(true);
	try {
		desktop.AddRecord(m_dbid, *bp);
	} catch( Barry::ReturnCodeError &rce ) {
		cerr << _C("Device exception: ") << rce.what() << endl;
		if( rce.IsReadOnly() ) {
			ShowReadOnlyMsg(parent, rce);
			return end();
		}

		throw;
	}
Barry::Verbose(iv);

	// update our copy of the record state table from device
	desktop.GetRecordStateTable(m_dbid, m_state);
cout << m_state << endl;

	// find our new record_id in list, to find the state index
	IndexType new_index;
	if( !m_state.GetIndex(record_id, &new_index) ) {
		throw std::logic_error("Need to reconnect for adding a record?");
	}

	// update new state_index in the data cache record
	p->SetIds(new_index, record_id);

	// add DataCachePtr to our own cache list
	m_records.push_front(p);

	// return iterator pointing to new record
	return begin();
}

DBCache::iterator DBCache::Add(wxWindow *parent,
				const Barry::TimeZones &zones,
				iterator copy_record)
{
	DataCachePtr p;

#undef HANDLE_BUILDER
#define HANDLE_BUILDER(tname) \
	if( m_dbname == Barry::tname::GetDBName() ) { \
		Barry::tname rec; \
		if( copy_record != end() ) { \
			RecordCache<Barry::tname> *rc = dynamic_cast<RecordCache<Barry::tname>* > (copy_record->get()); \
			if( rc ) { \
				rec = rc->GetRecord(); \
			} \
		} \
		p.reset( new RecordCache<Barry::tname>(0, rec) ); \
	}
	ALL_KNOWN_BUILDER_TYPES

	// anything else is not addable or buildable
	if( !p.get() ) {
		return end();
	}

	if( p->Edit(parent, true, zones) ) {
		return Add(parent, p);
	}
	else {
		return end();
	}
}

bool DBCache::OverwriteRecord(wxWindow *parent, iterator record)
{
	// see if this record has a builder
	Barry::Builder *bp = dynamic_cast<Barry::Builder*> ((*record).get());
	if( !bp )
		return false;

cout << "Changing device record with index: 0x" << hex << (*record)->GetStateIndex() << endl;
cout << m_state << endl;
	// update the device with new record data
	DesktopInstancePtr dip = m_tdesktop.Get();
	Barry::Mode::Desktop &desktop = dip->Desktop();
bool iv = Barry::IsVerbose();
Barry::Verbose(true);
	try {
		desktop.SetRecord(m_dbid, (*record)->GetStateIndex(), *bp);
	} catch( Barry::ReturnCodeError &rce ) {
		if( rce.IsReadOnly() ) {
			ShowReadOnlyMsg(parent, rce);
			return false;
		}

		throw;
	}
Barry::Verbose(iv);

	return true;
}

bool DBCache::DeleteAndAddRecord(wxWindow *parent, iterator record)
{
	// see if this record has a builder
	Barry::Builder *bp = dynamic_cast<Barry::Builder*> ((*record).get());
	if( !bp )
		return false;

cout << "Changing device record with index: 0x" << hex << (*record)->GetStateIndex() << endl;
cout << m_state << endl;
	// update the device with new record data
	DesktopInstancePtr dip = m_tdesktop.Get();
	Barry::Mode::Desktop &desktop = dip->Desktop();
bool iv = Barry::IsVerbose();
Barry::Verbose(true);
	try {
		desktop.DeleteRecord(m_dbid, (*record)->GetStateIndex());
		desktop.AddRecord(m_dbid, *bp);
	} catch( Barry::ReturnCodeError &rce ) {
		if( rce.IsReadOnly() ) {
			ShowReadOnlyMsg(parent, rce);
			return false;
		}

		throw;
	}
Barry::Verbose(iv);

	// update our copy of the record state table from device
	desktop.GetRecordStateTable(m_dbid, m_state);
cout << m_state << endl;

	// find our record_id in list, to find the state index
	IndexType new_index;
	if( !m_state.GetIndex((*record)->GetRecordId(), &new_index) ) {
		throw std::logic_error("DAA: Need to reconnect for adding a record?");
	}

	// update new state_index in the data cache record
	(*record)->SetIds(new_index, (*record)->GetRecordId());

	return true;
}

bool DBCache::Edit(wxWindow *parent,
		const Barry::TimeZones &zones,
		iterator record)
{
	if( record == end() )
		return false;

	if( (*record)->Edit(parent, true, zones) && (*record)->IsBuildable() ) {
		// see if this record is part of the Tasks database
		RecordCache<Barry::Task> *tp = dynamic_cast< RecordCache<Barry::Task>*> ((*record).get());

		if( tp ) {
			// yes, it is... the Tasks database has a bug
			// so we need to "edit" by deleting and adding
			// the record again
			return DeleteAndAddRecord(parent, record);
		}
		else {
			// use the normal code for all other records
			return OverwriteRecord(parent, record);
		}
	}
	else {
		return false;
	}
}

bool DBCache::Delete(wxWindow *parent, iterator record)
{
	if( record == end() )
		return false;

	// prompt user with Yes / No message
	wxString desc((*record)->GetDescription().c_str(), wxConvUTF8);
	int choice = wxMessageBox(_W("Delete this record?\n   ") + desc,
		_W("Record Delete"), wxYES_NO | wxICON_QUESTION, parent);

	// if no, return false
	if( choice != wxYES )
		return false;

cout << "Deleting device record with index: 0x" << hex << (*record)->GetStateIndex() << endl;
cout << m_state << endl;
	// delete record from device
	DesktopInstancePtr dip = m_tdesktop.Get();
	Barry::Mode::Desktop &desktop = dip->Desktop();
	desktop.DeleteRecord(m_dbid, (*record)->GetStateIndex());

	// remove record from cache list
	m_records.erase(record);
	return true;
}

// For Barry::AllRecordStore
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
void DBCache::operator() (const Barry::tname &rec) \
{ \
	DataCachePtr p( new RecordCache<Barry::tname>(m_index, rec) ); \
	m_records.push_front(p); \
}
ALL_KNOWN_PARSER_TYPES

// For Barry::Parser
void DBCache::ParseRecord(const Barry::DBData &data,
			const Barry::IConverter *ic)
{
	DataCachePtr p( new DBDataCache(m_index, data) );
	m_records.push_front(p);
}


//////////////////////////////////////////////////////////////////////////////
// DBMap

DBMap::DBMap(ThreadableDesktop &tdesktop)
	: m_tdesktop(tdesktop)
{
	if( pthread_mutex_init(&m_map_mutex, NULL) ) {
		throw Barry::Error(_C("Failed to create map mutex"));
	}

	if( pthread_mutex_init(&m_load_mutex, NULL) ) {
		throw Barry::Error(_C("Failed to create load mutex"));
	}
}

DBMap::DBCachePtr DBMap::LoadDBCache(const std::string &dbname)
{
	// first, check for pre-loaded data, before the load lock,
	// to make sure we return pre-loaded data with utmost haste
	{
		scoped_lock map_lock(m_map_mutex);

		MapType::iterator i = m_map.find(dbname);
		if( i != m_map.end() )
			return i->second;
	}

	// if not found, lock and load, but be careful, since we
	// don't want to open a window here for loading a db twice
	scoped_lock load_lock(m_load_mutex);

	// check again for pre-loaded data, since between
	// map.unlock and load.lock there could have been
	// another successful load
	{
		scoped_lock map_lock(m_map_mutex);

		MapType::iterator i = m_map.find(dbname);
		if( i != m_map.end() )
			return i->second;
	}

	// do the load, without map.lock, since this can take a
	// significant amount of time
	DBCachePtr p( new DBCache(m_tdesktop, dbname) );

	// lock once more to update the map, and then done
	scoped_lock map_lock(m_map_mutex);
	m_map[dbname] = p;
	return p;
}

DBMap::DBCachePtr DBMap::GetDBCache(const std::string &dbname)
{
	scoped_lock lock(m_map_mutex);

	MapType::iterator i = m_map.find(dbname);
	if( i != m_map.end() )
		return i->second;

	return DBCachePtr();
}

//////////////////////////////////////////////////////////////////////////////
// BrowseMode

BrowseMode::BrowseMode(wxWindow *parent, const ProbeResult &device)
	: m_parent(parent)
	, m_buildable(false)
	, m_editable(false)
	, m_cardable(false)
	, m_show_all(false)
{
	// create device identifying string
	m_device_id_str = wxString(device.GetDisplayName().c_str(), wxConvUTF8);

	//
	// connect to the device
	//
	m_con.reset( new GUIDesktopConnector(m_parent, "", "utf-8", device) );
	m_con->Reconnect(2);
	m_tdesktop.reset( new ThreadableDesktop(*m_con) );

	// keep our own copy, and sort by name for later
	m_dbdb = m_con->GetDesktop().GetDBDB();
	m_dbdb.SortByName();

	// store a copy of the time zone set for record editing
	if( TimeZones::IsLoadable(m_con->GetDesktop()) ) {
		// load time zones from device itself
		m_zones.reset( new TimeZones(m_con->GetDesktop()) );
	}
	else {
		// use static time zone table from Barry library
		m_zones.reset( new TimeZones );
	}

	CreateControls();

	// create our DBMap and give it the threadable desktop,
	// now that we're finished doing any desktop USB work
	m_dbmap.reset( new DBMap(*m_tdesktop) );

	//
	// From here down, we assume that our constructor succeeds, with
	// no exceptions!
	//

	// fire off a background thread to cache database records
	// in advance... if it fails, don't worry about it
	m_abort_flag = false;
	int ret = pthread_create(&m_cache_thread, NULL,
			&BrowseMode::FillCacheThread, this);
	if( ret != 0 )
		m_abort_flag = true;	// no need to join later

	// connect ourselves to the parent's event handling chain
	// do this last, so that we are guaranteed our destructor
	// will run, in case of exceptions
	m_parent->PushEventHandler(this);
}

BrowseMode::~BrowseMode()
{
	// unhook that event handler!
	m_parent->PopEventHandler();

	// make sure the cache thread is finished before we destroy it :-)
	if( !m_abort_flag ) {
		m_abort_flag = true;
		void *junk;
		pthread_join(m_cache_thread, &junk);
	}
}

std::string& GetDBName(Barry::DatabaseDatabase::Database &db)
{
	return db.Name;
}

void BrowseMode::SendStatusEvent(const std::string &dbname)
{
	wxCommandEvent event(BMET_LOAD_STATUS, wxID_ANY);
	event.SetEventObject(this);

	if( dbname.size() ) {
		wxString msg(_W("Loading: "));
		msg += wxString(dbname.c_str(), wxConvUTF8);
		event.SetString(msg);
	}
	else {
		event.SetString(_T(""));
	}

	AddPendingEvent(event);
}

void BrowseMode::CreateControls()
{
	m_top_sizer.reset( new wxBoxSizer(wxVERTICAL) );

	// make space for the main header, which is not part of our
	// work area
	m_top_sizer->AddSpacer(MAIN_HEADER_OFFSET);


	//
	// add list boxes to main area, the list_sizer
	//

	wxStaticBoxSizer *list_sizer = new wxStaticBoxSizer(wxHORIZONTAL,
		m_parent, m_device_id_str);

	// add database listctrl
	m_dbdb_list.reset (new wxListCtrl(m_parent, BrowseMode_DBDBList,
				wxDefaultPosition, wxDefaultSize,
				wxLC_REPORT | wxLC_SINGLE_SEL) ); //| wxLC_VRULES
//	int max_db_width = GetMaxWidth(m_dbdb_list.get(),
//		m_dbdb.Databases.begin(), m_dbdb.Databases.end(),
//		&GetDBName);
	list_sizer->Add( m_dbdb_list.get(), 4, wxEXPAND | wxALL, 4 );

	// add the record listctrl
	m_record_list.reset(new wxListCtrl(m_parent, BrowseMode_RecordList, 
				wxDefaultPosition, wxDefaultSize,
				wxLC_REPORT | wxLC_SINGLE_SEL) ); //| wxLC_VRULES
	list_sizer->Add( m_record_list.get(), 5, wxEXPAND | wxALL, 4 );

	// add list sizer to top sizer
	m_top_sizer->Add( list_sizer, 1, wxEXPAND | wxALL, 4 );

	//
	// add "show all" checkbox and load status static textbox, inside sizer
	//

	wxBoxSizer *status_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_show_all_checkbox.reset( new wxCheckBox(m_parent,
				BrowseMode_ShowAllCheckbox,
				_W("Show All Databases"),
				wxDefaultPosition, wxDefaultSize,
				wxCHK_2STATE) );
	status_sizer->Add( m_show_all_checkbox.get(), 0, wxEXPAND, 0 );
	m_show_all_checkbox->SetValue(m_show_all);

	status_sizer->AddStretchSpacer();

	m_load_status_text.reset( new wxStaticText(m_parent,
				BrowseMode_LoadStatusText,
				_T(""),
				wxDefaultPosition, wxSize(200, -1),
				wxST_NO_AUTORESIZE) );
	status_sizer->Add( m_load_status_text.get(), 0,
			wxEXPAND | wxALIGN_CENTRE_VERTICAL, 0 );

	m_top_sizer->Add( status_sizer, 0, wxEXPAND | wxALL, 4 );



	//
	// bottom buttons
	//

	// add bottom buttons - these go in the bottom FOOTER area
	// so their heights must be fixed to MAIN_HEADER_OFFSET
	// minus a border of 5px top and bottom
	wxSize footer(75, MAIN_HEADER_OFFSET - 5 - 5);
	wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	m_import_record_button.reset( new wxButton(m_parent,
				BrowseMode_ImportRecordButton, _W("Import..."),
				wxDefaultPosition, footer) );
	m_export_record_button.reset( new wxButton(m_parent,
				BrowseMode_ExportRecordButton, _W("Export..."),
				wxDefaultPosition, footer) );
	m_add_record_button.reset( new wxButton(m_parent,
				BrowseMode_AddRecordButton, _W("Add..."),
				wxDefaultPosition, footer) );
	m_copy_record_button.reset( new wxButton(m_parent,
				BrowseMode_CopyRecordButton, _W("Copy..."),
				wxDefaultPosition, footer) );
	m_edit_record_button.reset( new wxButton(m_parent,
				BrowseMode_EditRecordButton, _W("Edit..."),
				wxDefaultPosition, footer));
	m_delete_record_button.reset( new wxButton(m_parent,
				BrowseMode_DeleteRecordButton, _W("Delete..."),
				wxDefaultPosition, footer) );
	buttons->Add(m_import_record_button.get(), 0, wxRIGHT, 5);
	buttons->Add(m_export_record_button.get(), 0, wxRIGHT, 5);
	buttons->Add(m_add_record_button.get(), 0, wxRIGHT, 5);
	buttons->Add(m_copy_record_button.get(), 0, wxRIGHT, 5);
	buttons->Add(m_edit_record_button.get(), 0, wxRIGHT, 5);
	buttons->Add(m_delete_record_button.get(), 0, wxRIGHT, 5);
	m_top_sizer->Add(buttons, 0, wxALL | wxALIGN_RIGHT, 5);

	//
	// recalc size of children and add columns
	//
	wxSize client_size = m_parent->GetClientSize();
	m_top_sizer->SetDimension(0, 0,
		client_size.GetWidth(), client_size.GetHeight());

	// m_dbdb_list
	wxSize dbdb_size = m_dbdb_list->GetClientSize();
	int scroll_width = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
	int size = dbdb_size.GetWidth() - scroll_width;
	m_dbdb_list->InsertColumn(0, _W("Databases"), wxLIST_FORMAT_LEFT,
		size * 0.80);
	m_dbdb_list->InsertColumn(1, _W("Count"), wxLIST_FORMAT_LEFT,
		size * 0.20 + scroll_width); // add back the scroll width
					// so it doesn't look half-baked when
					// there is no scroll bar


	// m_record_list
	wxSize record_size = m_record_list->GetClientSize();
	m_record_list->InsertColumn(0, _W("Record Description"),
		wxLIST_FORMAT_LEFT, record_size.GetWidth());


	//
	// add data
	//
	FillDBDBList();

/*
	// attempt to re-select the devices as we last saw them
	ReselectDevices(m_device_set->String2Subset(wxGetApp().GetGlobalConfig().GetKey("SelectedDevices")));
	UpdateButtons();

*/
}

int BrowseMode::GUItoDBDBIndex(int gui_index)
{
	if( m_show_all )
		return gui_index;

	DatabaseDatabase::DatabaseArrayType::const_iterator
		i = m_dbdb.Databases.begin(), e = m_dbdb.Databases.end();
	for( int index = 0; i != e; ++i, index++ ) {
		// only bump index on the parsable databases
		if( !m_show_all && !IsParsable(i->Name) )
			continue;

		if( !gui_index )
			return index;

		gui_index--;
	}

	// error
	return -1;
}

void BrowseMode::FillDBDBList()
{
	// start fresh
	m_dbdb_list->DeleteAllItems();

	DatabaseDatabase::DatabaseArrayType::const_iterator
		i = m_dbdb.Databases.begin(), e = m_dbdb.Databases.end();
	for( int index = 0; i != e; ++i, index++ ) {
		// Only show parsable databases, depending on GUI
		if( !m_show_all && !IsParsable(i->Name) )
			continue;

		// Database Name
		wxString text(i->Name.c_str(), wxConvUTF8);
		long item = m_dbdb_list->InsertItem(index, text);

		// Record Count
		ostringstream oss;
		oss << dec << i->RecordCount;
		text = wxString(oss.str().c_str(), wxConvUTF8);
		m_dbdb_list->SetItem(item, 1, text);
	}

	UpdateButtons();
}

void BrowseMode::FillRecordList(const std::string &dbname)
{
	try {

		// start fresh
		m_record_list->DeleteAllItems();

		// grab our DB
		DBMap::DBCachePtr db = m_dbmap->LoadDBCache(dbname);

		// cycle through the cache, and insert the descriptions
		// given for each record
		DBCache::const_iterator b = db->begin(), e = db->end();
		for( int index = 0; b != e; ++b, index++ ) {
			wxString text((*b)->GetDescription().c_str(), wxConvUTF8);
			//long item =
				m_record_list->InsertItem(index, text);
		}

	} catch( Barry::Error &be ) {
		cerr << be.what() << endl;
	}
}

void BrowseMode::UpdateButtons()
{
	int selected_count = m_record_list->GetSelectedItemCount();

	// can only import if this is a cardable DB
	m_import_record_button->Enable(m_cardable);

	// can only export if this is a cardable DB and only 1 selected
	m_export_record_button->Enable(m_cardable && selected_count == 1);

	// can only add if we have a builder and dialog for this record type
	m_add_record_button->Enable(m_buildable && m_editable);

	// can only copy or edit if we have a builder, a dialog, and
	// only 1 is selected
	m_copy_record_button->Enable(
		m_buildable && m_editable && selected_count == 1);
	m_edit_record_button->Enable(
		m_buildable && m_editable && selected_count == 1);

	// can only delete if something is selected
	m_delete_record_button->Enable(selected_count > 0);
}

void BrowseMode::FillCache()
{
	// create a copy of the dbdb, and sort in ascending order of
	// record count, so the last db loaded is the longest...
	// hopefully this makes the UI more user-friendly and responsive
	DatabaseDatabase dbdb = m_dbdb;
	dbdb.SortByRecordCount();

	// cycle through the dbdb and load all Parsable databases
	DatabaseDatabase::DatabaseArrayType::const_iterator
		i = dbdb.Databases.begin(), e = dbdb.Databases.end();
	for( ; i != e; ++i ) {
		if( IsParsable(i->Name) ) try {
			SendStatusEvent(i->Name);
			m_dbmap->LoadDBCache(i->Name);
		} catch( Barry::Error &be ) {
			cerr << be.what() << endl;
		}

		if( m_abort_flag )
			break;
	}

	SendStatusEvent("");

	// finished
	m_abort_flag = true;
}

void* BrowseMode::FillCacheThread(void *bobj)
{
	BrowseMode *bm = (BrowseMode*) bobj;
	bm->FillCache();
	return NULL;
}

void BrowseMode::OnDBDBListSelChange(wxListEvent &event)
{
	wxBusyCursor wait;
	int index = GUItoDBDBIndex(event.GetIndex());
	m_current_dbname = m_dbdb.Databases.at(index).Name;
	m_buildable = ::IsBuildable(m_current_dbname);
	m_editable = ::IsEditable(m_current_dbname);
	m_cardable = ::IsCardable(m_current_dbname);
	m_current_record_item = -1;

	FillRecordList(m_current_dbname);
	UpdateButtons();
}

void BrowseMode::OnRecordListSelChange(wxListEvent &event)
{
	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr p = m_dbmap->GetDBCache(m_current_dbname);
	if( !p.get() )
		return;

	// grab the record list index
	m_current_record_item = event.GetIndex();
//	m_current_record_item = m_record_list->GetNextItem(
//		m_current_record_item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

	UpdateButtons();
}

void BrowseMode::OnRecordListActivated(wxListEvent &event)
{
	wxCommandEvent ce;
	OnEditRecord(ce);
}

void BrowseMode::OnShowAll(wxCommandEvent &event)
{
	m_show_all = !m_show_all;
	FillDBDBList();
}

template <class SyncT>
bool CheckTypes(wxWindow *parent,
		const string &dbname,
		const vector<string> &types)
{
	if( !MimeBuilder::IsMember(SyncT::GetVName(), types) ) {
		CategoryList tlist;
		tlist = types;
		string tslist;
		tlist.CategoryList2Str(tslist);

		wxString msg = wxString::Format(_W("Card file type (%s) does not match record you are trying to add (%s)."),
			wxString(tslist.c_str(), wxConvUTF8).c_str(),
			wxString(dbname.c_str(), wxConvUTF8).c_str());
		wxMessageBox(msg, _W("Invalid Card Type"),
			wxOK | wxICON_INFORMATION, parent);
		return false;
	}

	return true;
}

void BrowseMode::OnImportRecord(wxCommandEvent &event)
{
	string file_types;
	if( !IsCardable(m_current_dbname, &file_types) )
		return;

	// we are loading files here, so also allow *.*
	file_types += "|";
	file_types += _C("All files");
	file_types += " (*.*)|*.*";

	wxString file_filter(file_types.c_str(), wxConvUTF8);

	wxFileDialog dlg(m_parent, _W("Load Record..."), _T(""), _T(""),
		file_filter,
		wxFD_OPEN | wxFD_PREVIEW);
	if( dlg.ShowModal() != wxID_OK )
		return;

	// open file
	ifstream ifs(dlg.GetPath().utf8_str());
	string vrec;
	vector<string> types;
	if( !MimeBuilder::ReadMimeRecord(ifs, vrec, types) ) {
		wxMessageBox(_W("No card data found in: ") + dlg.GetPath(),
			_W("Import Read Error"),
			wxOK | wxICON_ERROR, m_parent);
		return;
	}

	DataCachePtr rp;
	string convert_error;
	try {
		const string &dn = m_current_dbname;

		// and read per record type:
		if( m_current_dbname == Contact::GetDBName() ) {
			if( !CheckTypes<Sync::vCard>(m_parent, dn, types) )
				return;
			Sync::vCard vcard;
			Contact rec = vcard.ToBarry(vrec.c_str(), 0);
			rp.reset( new RecordCache<Contact>(0, rec) );
		}
		else if( m_current_dbname == Calendar::GetDBName() ) {
			if( !CheckTypes<Sync::vCalendar>(m_parent, dn, types) )
				return;
			Sync::vTimeConverter vtc;
			Sync::vCalendar vcal(vtc);
			Calendar rec = vcal.ToBarry(vrec.c_str(), 0);
			rp.reset( new RecordCache<Calendar>(0, rec) );
		}
		else if( m_current_dbname == Memo::GetDBName() ) {
			if( !CheckTypes<Sync::vJournal>(m_parent, dn, types) )
				return;
			Sync::vTimeConverter vtc;
			Sync::vJournal vjournal(vtc);
			Memo rec = vjournal.ToBarry(vrec.c_str(), 0);
			rp.reset( new RecordCache<Memo>(0, rec) );
		}
		else if( m_current_dbname == Task::GetDBName() ) {
			if( !CheckTypes<Sync::vTodo>(m_parent, dn, types) )
				return;
			Sync::vTimeConverter vtc;
			Sync::vTodo vtodo(vtc);
			Task rec = vtodo.ToBarry(vrec.c_str(), 0);
			rp.reset( new RecordCache<Task>(0, rec) );
		}
	} catch( Barry::ConvertError &ce ) {
		convert_error = ce.what();
	}

	if( convert_error.size() ) {
		wxString msg = wxString::Format(_W("Unable to import selected file: %s"), wxString(convert_error.c_str(), wxConvUTF8).c_str());
		wxMessageBox(msg, _W("Import Error"), wxOK | wxICON_ERROR,
			m_parent);
		return;
	}

	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr dbp = m_dbmap->GetDBCache(m_current_dbname);
	if( !dbp.get() ) {
		wxMessageBox(_W("Internal pointer error: cannot find DBCachePtr for: ") + wxString(m_current_dbname.c_str(), wxConvUTF8),
			_W("Internal Error"),
			wxOK | wxICON_ERROR, m_parent);
		return;
	}

	DBCache::iterator i = dbp->Add(m_parent, rp);
	if( i != dbp->end() ) {
		wxString text((*i)->GetDescription().c_str(), wxConvUTF8);

		// insert new record in same spot as DBCache has it
		m_current_record_item = dbp->GetIndex(i);
		m_record_list->InsertItem(m_current_record_item, text);
	}
	else {
		wxMessageBox(_W("Internal error: cannot add record to DBCache"),
			_W("Internal Error"),
			wxOK | wxICON_ERROR, m_parent);
		return;
	}
}

void BrowseMode::OnExportRecord(wxCommandEvent &event)
{
	string file_types;
	if( !IsCardable(m_current_dbname, &file_types) )
		return;

	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr p = m_dbmap->GetDBCache(m_current_dbname);
	if( !p.get() )
		return;

	DBCache::iterator i = p->Get(m_current_record_item);
	string vdata;
	if( (*i)->Card(m_parent, vdata) ) {
		MimeExportDlg dlg(m_parent, vdata, file_types);
		dlg.ShowModal();
	}
}

void BrowseMode::OnAddRecord(wxCommandEvent &event)
{
	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr p = m_dbmap->GetDBCache(m_current_dbname);
	if( !p.get() )
		return;

	DBCache::iterator i = p->Add(m_parent, *m_zones, p->end());
	if( i != p->end() ) {
		wxString text((*i)->GetDescription().c_str(), wxConvUTF8);

		// insert new record in same spot as DBCache has it
		m_current_record_item = p->GetIndex(i);
		m_record_list->InsertItem(m_current_record_item, text);
	}
}

void BrowseMode::OnCopyRecord(wxCommandEvent &event)
{
	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr p = m_dbmap->GetDBCache(m_current_dbname);
	if( !p.get() )
		return;

	DBCache::iterator source = p->Get(m_current_record_item);
	DBCache::iterator i = p->Add(m_parent, *m_zones, source);
	if( i != p->end() ) {
		wxString text((*i)->GetDescription().c_str(), wxConvUTF8);

		// insert new record in same spot as DBCache has it
		m_current_record_item = p->GetIndex(i);
		m_record_list->InsertItem(m_current_record_item, text);
	}
}

void BrowseMode::OnEditRecord(wxCommandEvent &event)
{
	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr p = m_dbmap->GetDBCache(m_current_dbname);
	if( !p.get() )
		return;

	DBCache::iterator i = p->Get(m_current_record_item);
	if( p->Edit(m_parent, *m_zones, i) ) {
		wxString text((*i)->GetDescription().c_str(), wxConvUTF8);
		m_record_list->SetItem(m_current_record_item, 0, text);
	}
}

void BrowseMode::OnDeleteRecord(wxCommandEvent &event)
{
	// grab the cache for the current database... Get is ok here,
	// since the cache is already loaded by the main db list
	DBMap::DBCachePtr p = m_dbmap->GetDBCache(m_current_dbname);
	if( !p.get() )
		return;

	DBCache::iterator i = p->Get(m_current_record_item);
	if( p->Delete(m_parent, i) ) {
		m_record_list->DeleteItem(m_current_record_item);
	}
}

void BrowseMode::OnStatusEvent(wxCommandEvent &event)
{
	m_load_status_text->SetLabel(event.GetString());
}

