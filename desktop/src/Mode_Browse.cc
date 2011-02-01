///
/// \file	Mode_Browse.cc
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

#include "Mode_Browse.h"
#include "BaseFrame.h"
#include "windowids.h"
#include <sstream>
#include <iomanip>

using namespace std;
using namespace Barry;

BEGIN_EVENT_TABLE(BrowseMode, wxEvtHandler)
	EVT_LIST_ITEM_SELECTED(BrowseMode_DBDBList,
				BrowseMode::OnDBDBListSelChange)
	EVT_CHECKBOX	(BrowseMode_ShowAllCheckbox,
				BrowseMode::OnShowAll)
	EVT_BUTTON	(BrowseMode_AddRecordButton,
				BrowseMode::OnAddRecord)
	EVT_BUTTON	(BrowseMode_EditRecordButton,
				BrowseMode::OnEditRecord)
	EVT_BUTTON	(BrowseMode_DeleteRecordButton,
				BrowseMode::OnDeleteRecord)
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////////////
// Standalone functions

bool EditRecord(wxWindow *parent, bool editable, Barry::Contact &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Bookmark &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Calendar &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::CalendarAll &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::ContentStore &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Folder &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Memo &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Message &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::CallLog &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::PINMessage &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::SavedMessage &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::ServiceBook &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Sms &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Task &rec)
{
	return false;
}

bool EditRecord(wxWindow *parent, bool editable, Barry::Timezone &rec)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////////
// DBDataCache

DBDataCache::DBDataCache(DataCache::IndexType index, const Barry::DBData &raw)
	: DataCache(index)
	, m_raw(raw)
{
}

void DBDataCache::Edit(wxWindow *parent, bool editable)
{
}

std::string DBDataCache::GetDescription() const
{
	return "raw data";
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

	// FIXME - sort here?
}

DBCache::~DBCache()
{
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
		throw Barry::Error("Failed to create map mutex");
	}
}

DBMap::DBCachePtr DBMap::GetDBCache(const std::string &dbname)
{
	scoped_lock lock(m_map_mutex);

	MapType::iterator i = m_map.find(dbname);
	if( i != m_map.end() )
		return i->second;

	// do not unlock here, since we only want to load this
	// data once, not provide a window for loading it twice

	// not found, time to load it up
	DBCachePtr p( new DBCache(m_tdesktop, dbname) );
	m_map[dbname] = p;
	return p;
}

//////////////////////////////////////////////////////////////////////////////
// BrowseMode

BrowseMode::BrowseMode(wxWindow *parent, const ProbeResult &device)
	: m_parent(parent)
	, m_buildable(false)
	, m_show_all(false)
{
	// create device identifying string
	ostringstream oss;
	oss << device.m_pin.Str();
	if( device.m_cfgDeviceName.size() ) {
		oss << " (" << device.m_cfgDeviceName << ")";
	}
	m_device_id_str = wxString(oss.str().c_str(), wxConvUTF8);

	// exceptions are used here... is that a good idea?
	m_ic.reset( new IConverter("utf-8", true) );

	//
	// connect to the device
	//
	m_con.reset( new Controller(device) );
	m_desktop.reset( new Barry::Mode::Desktop(*m_con, *m_ic) );
	m_tdesktop.reset( new ThreadableDesktop(*m_desktop, *m_ic) );

	string password;
	bool first = true;
	while(1) {
		wxString prompt;

		try {
			if( first ) {
				first = false;
				m_desktop->Open(password.c_str());
			}
			else {
				m_desktop->RetryPassword(password.c_str());
			}
			break;
		}
		catch( BadPassword &bp ) {
			if( bp.out_of_tries() ) {
				throw;
			}

			// create prompt based on exception data
			ostringstream oss;
			oss << "Please enter device password: ("
			    << bp.remaining_tries()
			    << " tries remaining)";
			prompt = wxString(oss.str().c_str(), wxConvUTF8);

			// fall through to password prompt
		}

		// ask use for device password
		wxString pass = wxGetPasswordFromUser(prompt,
			_T("Device Password"), _T(""), m_parent);

		password = pass.utf8_str();
	}

	// keep our own copy, and sort by name for later
	m_dbdb = m_desktop->GetDBDB();
	m_dbdb.SortByName();

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
	// add "show all" checkbox
	//

	m_show_all_checkbox.reset( new wxCheckBox(m_parent,
				BrowseMode_ShowAllCheckbox,
				_T("Show All Databases"),
				wxDefaultPosition, wxDefaultSize,
				wxCHK_2STATE) );
	m_top_sizer->Add( m_show_all_checkbox.get(), 0, wxEXPAND | wxALL, 4 );
	m_show_all_checkbox->SetValue(m_show_all);

	//
	// bottom buttons
	//

	// add bottom buttons - these go in the bottom FOOTER area
	// so their heights must be fixed to MAIN_HEADER_OFFSET
	// minus a border of 5px top and bottom
	wxSize footer(-1, MAIN_HEADER_OFFSET - 5 - 5);
	wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	m_add_record_button.reset( new wxButton(m_parent,
				BrowseMode_AddRecordButton, _T("Add..."),
				wxDefaultPosition, footer) );
	m_edit_record_button.reset( new wxButton(m_parent,
				BrowseMode_EditRecordButton, _T("Edit..."),
				wxDefaultPosition, footer));
	m_delete_record_button.reset( new wxButton(m_parent,
				BrowseMode_DeleteRecordButton, _T("Delete..."),
				wxDefaultPosition, footer) );
	buttons->Add(m_add_record_button.get(), 0, wxRIGHT, 5 );
	buttons->Add(m_edit_record_button.get(), 0, wxRIGHT, 5 );
	buttons->Add(m_delete_record_button.get(), 0, wxRIGHT, 5 );
	m_top_sizer->Add(buttons, 0, wxALL | wxALIGN_RIGHT, 5 );

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
	m_dbdb_list->InsertColumn(0, _T("Databases"), wxLIST_FORMAT_LEFT,
		size * 0.80);
	m_dbdb_list->InsertColumn(1, _T("Count"), wxLIST_FORMAT_LEFT,
		size * 0.20 + scroll_width); // add back the scroll width
					// so it doesn't look half-baked when
					// there is no scroll bar


	// m_record_list
	wxSize record_size = m_record_list->GetClientSize();
	m_record_list->InsertColumn(0, _T("Record Description"),
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
		DBMap::DBCachePtr db = m_dbmap->GetDBCache(dbname);

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

	// can only add if we have a builder for this record type
	m_add_record_button->Enable(m_buildable);
	// can only edit if we have a builder, and if only 1 is selected
	m_edit_record_button->Enable(m_buildable && selected_count == 1);
	// can only delete if something is selected
	m_delete_record_button->Enable(selected_count > 0);
}

void BrowseMode::FillCache()
{
	// cycle through the dbdb and load all Parsable databases
	DatabaseDatabase::DatabaseArrayType::const_iterator
		i = m_dbdb.Databases.begin(), e = m_dbdb.Databases.end();
	for( ; i != e; ++i ) {
		if( IsParsable(i->Name) ) try {
			m_dbmap->GetDBCache(i->Name);
		} catch( Barry::Error &be ) {
			cerr << be.what() << endl;
		}

		if( m_abort_flag )
			break;
	}

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
	FillRecordList(m_dbdb.Databases.at(index).Name);
	UpdateButtons();
}

void BrowseMode::OnShowAll(wxCommandEvent &event)
{
	m_show_all = !m_show_all;
	FillDBDBList();
}

void BrowseMode::OnAddRecord(wxCommandEvent &event)
{
}

void BrowseMode::OnEditRecord(wxCommandEvent &event)
{
}

void BrowseMode::OnDeleteRecord(wxCommandEvent &event)
{
}

