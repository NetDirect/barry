///
/// \file	TaskEditDlg.cc
///		Dialog class to handle the editing of the Task record
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "TaskEditDlg.h"
#include "windowids.h"
#include <wx/valgen.h>
#include "wxval.h"

using namespace std;
using namespace Barry;

// begin wxGlade: ::extracode
// end wxGlade


//////////////////////////////////////////////////////////////////////////////
// Helper functions

void MakeRecent(wxCheckBox *check, wxDatePickerCtrl *picker)
{
	wxDateTime when = picker->GetValue();
	if( check->IsChecked() &&
	    (!when.IsValid() ||
		when < wxDateTime(1, wxDateTime::Jan, 1975, 0, 0, 0)) )
	{
		when = wxDateTime::Now();
		picker->SetValue(when);
	}
}


//////////////////////////////////////////////////////////////////////////////
// TaskEditDlg class

TaskEditDlg::TaskEditDlg(wxWindow* parent,
			Barry::Task &rec,
			bool editable,
			const Barry::TimeZones *device_zones)
	: wxDialog(parent, Dialog_TaskEdit, _T("Task Record"))
	, m_zones(device_zones ? device_zones : &m_static_zones)
	, m_rec(rec)
	, m_reminder_hours(0)
	, m_reminder_minutes(0)
	, m_interval(0)
	, m_relative_date(false)
{
	// set all weekday 'bits' to false
	for( int i = 0; i < 7; i++ )
		m_weekdays[i] = false;

	if( editable ) {
		bottom_buttons = CreateButtonSizer(wxOK | wxCANCEL);
	}
	else {
		bottom_buttons = CreateButtonSizer(wxCANCEL);
	}

	// begin wxGlade: TaskEditDlg::TaskEditDlg
	label_1 = new wxStaticText(this, wxID_ANY, wxT("Task:"));
	m_TaskSummary = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_1 = new wxStaticLine(this, wxID_ANY);
	label_2 = new wxStaticText(this, wxID_ANY, wxT("Status:"));
	const wxString m_StatusChoice_choices[] = {
        wxT("Not Started"),
        wxT("In Progress"),
        wxT("Completed"),
        wxT("Waiting"),
        wxT("Deferred")
    };
	m_StatusChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 5, m_StatusChoice_choices, 0);
	label_9 = new wxStaticText(this, wxID_ANY, wxT("Priority:"));
	const wxString m_PriorityChoice_choices[] = {
        wxT("High"),
        wxT("Normal"),
        wxT("Low")
    };
	m_PriorityChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 3, m_PriorityChoice_choices, 0);
	label_5 = new wxStaticText(this, wxID_ANY, wxT("Due:"));
	m_DueCheck = new wxCheckBox(this, Dialog_TaskEdit_DueCheck, wxEmptyString);
	m_DueDateCtrl = new wxDatePickerCtrl(this, Dialog_TaskEdit_DueDateCtrl, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	m_DueHoursSpinner = new wxSpinCtrl(this, Dialog_TaskEdit_DueHoursSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 23);
	label_11 = new wxStaticText(this, wxID_ANY, wxT(":"));
	m_DueMinutesSpinner = new wxSpinCtrl(this, Dialog_TaskEdit_DueMinutesSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 59);
	label_8 = new wxStaticText(this, wxID_ANY, wxT("Time Zone:"));
	const wxString m_TimezoneChoice_choices[] = {
        wxT("System Time Zone")
    };
	m_TimezoneChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 1, m_TimezoneChoice_choices, 0);
	label_10 = new wxStaticText(this, wxID_ANY, wxT("Reminder:"));
	m_ReminderCheck = new wxCheckBox(this, Dialog_TaskEdit_ReminderCheck, wxEmptyString);
	m_ReminderDateCtrl = new wxDatePickerCtrl(this, Dialog_TaskEdit_ReminderDateCtrl);
	m_ReminderHoursSpinner = new wxSpinCtrl(this, Dialog_TaskEdit_ReminderHoursSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999);
	label_6 = new wxStaticText(this, wxID_ANY, wxT(":"));
	m_ReminderMinutesSpinner = new wxSpinCtrl(this, Dialog_TaskEdit_ReminderMinutesSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59);
	static_line_2 = new wxStaticLine(this, wxID_ANY);
	label_18 = new wxStaticText(this, wxID_ANY, wxT("Recurrence:"));
	const wxString m_RecurrenceChoice_choices[] = {
        wxT("None"),
        wxT("Daily"),
        wxT("Weekly"),
        wxT("Monthly"),
        wxT("Yearly")
    };
	m_RecurrenceChoice = new wxChoice(this, Dialog_TaskEdit_RecurrenceChoice, wxDefaultPosition, wxDefaultSize, 5, m_RecurrenceChoice_choices, 0);
	RecurIntervalLabel = new wxStaticText(this, wxID_ANY, wxT("Interval:"));
	RecurIntervalLabelB = new wxStaticText(this, wxID_ANY, wxT("Every"));
	m_IntervalSpinner = new wxSpinCtrl(this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999);
	m_IntervalUnitLabel = new wxStaticText(this, wxID_ANY, wxT("days? weeks? months?"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	RecurDaysLabel = new wxStaticText(this, wxID_ANY, wxT("Days:"));
	m_SunCheck = new wxCheckBox(this, wxID_ANY, wxT("S"));
	m_MonCheck = new wxCheckBox(this, wxID_ANY, wxT("M"));
	m_TueCheck = new wxCheckBox(this, wxID_ANY, wxT("T"));
	m_WedCheck = new wxCheckBox(this, wxID_ANY, wxT("W"));
	m_ThuCheck = new wxCheckBox(this, wxID_ANY, wxT("T"));
	m_FriCheck = new wxCheckBox(this, wxID_ANY, wxT("F"));
	m_SatCheck = new wxCheckBox(this, wxID_ANY, wxT("S"));
	RecurRelativeDateLabel = new wxStaticText(this, wxID_ANY, wxT("Relative Date:"));
	m_RelativeDateCheck = new wxCheckBox(this, wxID_ANY, wxEmptyString);
	RecurEndDateLabel = new wxStaticText(this, wxID_ANY, wxT("End Date:"));
	m_NeverEndsCheck = new wxCheckBox(this, Dialog_TaskEdit_NeverEndsCheck, wxT("Never ends"));
	m_RecurEndDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	static_line_3 = new wxStaticLine(this, wxID_ANY);
	label_4 = new wxStaticText(this, wxID_ANY, wxT("Categories:"));
	m_CategoriesText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_3 = new wxStaticText(this, wxID_ANY, wxT("Notes:"));
	m_NotesText = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

	set_properties();
	do_layout();
	// end wxGlade

	m_top_sizer->Add(bottom_buttons, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);

	// fill the time zone control with real time zones
	m_TimezoneChoice->Clear();
	m_TimezoneChoice->Append(wxT("Assume Local Timezone"), (void*)0);
	Barry::TimeZones::const_iterator b, e;
	for( b = m_zones->begin(), e = m_zones->end(); b != e; ++b ) {
		m_TimezoneChoice->Append(
			wxString(b->GetDescription().c_str(), wxConvUTF8),
			(void*) &b->Index);
	}
	m_TimezoneChoice->SetSelection(0);

	// layout again, in case sizes are different
	RedoLayout();
}

void TaskEditDlg::RedoLayout()
{
	m_top_sizer->Fit(this);
	Layout();
}


BEGIN_EVENT_TABLE(TaskEditDlg, wxDialog)
	// begin wxGlade: TaskEditDlg::event_table
	EVT_CHECKBOX(Dialog_TaskEdit_DueCheck, TaskEditDlg::OnDueCheck)
	EVT_CHECKBOX(Dialog_TaskEdit_ReminderCheck, TaskEditDlg::OnReminderCheck)
	EVT_CHOICE(Dialog_TaskEdit_RecurrenceChoice, TaskEditDlg::OnRecurrenceChoice)
	EVT_CHECKBOX(Dialog_TaskEdit_NeverEndsCheck, TaskEditDlg::OnEndDateCheckbox)
	// end wxGlade
END_EVENT_TABLE();


// wxGlade: add TaskEditDlg event handlers

void TaskEditDlg::OnDueCheck(wxCommandEvent &event)
{
	EnableDueDate(m_DueCheck->IsChecked());

	// make sure the first date is in a recent range, if not previously
	// valid...
	MakeRecent(m_DueCheck, m_DueDateCtrl);
}

void TaskEditDlg::OnRecurrenceChoice(wxCommandEvent &event)
{
	TransferDataFromWindow();
	EnableRecurMode(m_rec.Recurring);
}

void TaskEditDlg::OnEndDateCheckbox(wxCommandEvent &event)
{
	m_RecurEndDateCtrl->Enable( !m_NeverEndsCheck->IsChecked() );
}

void TaskEditDlg::OnReminderCheck(wxCommandEvent &event)
{
	EnableReminderDate(m_ReminderCheck->IsChecked());

	// make sure the first date is in a recent range, if not previously
	// valid...
	MakeRecent(m_ReminderCheck, m_ReminderDateCtrl);
}

void TaskEditDlg::set_properties()
{
	// begin wxGlade: TaskEditDlg::set_properties
	SetTitle(wxT("Task Event"));
	m_TaskSummary->SetFocus();
	m_TaskSummary->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Summary)));
	m_StatusChoice->SetSelection(0);
	m_PriorityChoice->SetSelection(1);
	m_DueDateCtrl->SetMinSize(wxSize(110, -1));
	m_DueDateCtrl->SetValidator(DateTimeValidator(&m_DueDateObj.m_date));
	m_DueHoursSpinner->SetMinSize(wxSize(45, -1));
	m_DueHoursSpinner->SetValidator(wxGenericValidator(&m_DueDateObj.m_hour));
	m_DueMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_DueMinutesSpinner->SetValidator(wxGenericValidator(&m_DueDateObj.m_min));
	m_TimezoneChoice->SetSelection(0);
	m_ReminderDateCtrl->SetMinSize(wxSize(110, -1));
	m_ReminderDateCtrl->SetValidator(DateTimeValidator(&m_ReminderDateObj.m_date));
	m_ReminderHoursSpinner->SetMinSize(wxSize(45, -1));
	m_ReminderHoursSpinner->SetToolTip(wxT("Set Reminder to 0 to disable"));
	m_ReminderHoursSpinner->SetValidator(wxGenericValidator(&m_ReminderDateObj.m_hour));
	m_ReminderMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_ReminderMinutesSpinner->SetToolTip(wxT("Set Reminder to 0 to disable"));
	m_ReminderMinutesSpinner->SetValidator(wxGenericValidator(&m_ReminderDateObj.m_min));
	m_RecurrenceChoice->SetValidator(wxGenericValidator(&m_recur_choice));
	m_RecurrenceChoice->SetSelection(0);
	m_IntervalSpinner->SetMinSize(wxSize(45, -1));
	m_IntervalSpinner->SetValidator(wxGenericValidator(&m_interval));
	m_SunCheck->SetValidator(wxGenericValidator(&m_weekdays[0]));
	m_MonCheck->SetValidator(wxGenericValidator(&m_weekdays[1]));
	m_TueCheck->SetValidator(wxGenericValidator(&m_weekdays[2]));
	m_WedCheck->SetValidator(wxGenericValidator(&m_weekdays[3]));
	m_ThuCheck->SetValidator(wxGenericValidator(&m_weekdays[4]));
	m_FriCheck->SetValidator(wxGenericValidator(&m_weekdays[5]));
	m_SatCheck->SetValidator(wxGenericValidator(&m_weekdays[6]));
	RecurRelativeDateLabel->SetToolTip(wxT("Relative monthly or yearly dates take the weekday of the start date into account. (eg. every first Sunday of month)"));
	m_RelativeDateCheck->SetToolTip(wxT("Relative monthly or yearly dates take the weekday of the start date into account. (eg. every first Sunday of month)"));
	m_RelativeDateCheck->SetValidator(wxGenericValidator(&m_relative_date));
	m_NeverEndsCheck->SetValidator(wxGenericValidator(&m_rec.Perpetual));
	m_NeverEndsCheck->SetValue(1);
	m_RecurEndDateCtrl->SetMinSize(wxSize(110, -1));
	m_RecurEndDateCtrl->Enable(false);
	m_RecurEndDateCtrl->SetValidator(DateTimeValidator(&m_RecurEndDateObj.m_date));
	m_CategoriesText->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_categories)));
	m_NotesText->SetMinSize(wxSize(-1, 71));
	m_NotesText->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Notes)));
	// end wxGlade
}

void TaskEditDlg::do_layout()
{
	// begin wxGlade: TaskEditDlg::do_layout
	wxBoxSizer* sizer_surround = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* grid_sizer_3 = new wxFlexGridSizer(2, 2, 5, 5);
	wxFlexGridSizer* grid_sizer_4 = new wxFlexGridSizer(5, 2, 5, 5);
	wxBoxSizer* sizer_8 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* m_DaysCtrlsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* m_IntervalCtrlsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_2 = new wxFlexGridSizer(10, 2, 5, 5);
	wxBoxSizer* sizer_5_copy = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_1 = new wxFlexGridSizer(2, 2, 5, 5);
	grid_sizer_1->Add(label_1, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(m_TaskSummary, 0, wxEXPAND, 0);
	grid_sizer_1->AddGrowableCol(1);
	sizer_1->Add(grid_sizer_1, 0, wxEXPAND, 0);
	sizer_1->Add(static_line_1, 0, wxALL|wxEXPAND, 5);
	grid_sizer_2->Add(label_2, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_2->Add(m_StatusChoice, 0, 0, 0);
	grid_sizer_2->Add(label_9, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_2->Add(m_PriorityChoice, 0, 0, 0);
	grid_sizer_2->Add(label_5, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_3->Add(m_DueCheck, 0, 0, 0);
	sizer_3->Add(m_DueDateCtrl, 0, 0, 0);
	sizer_3->Add(20, 20, 0, 0, 0);
	sizer_3->Add(m_DueHoursSpinner, 0, 0, 0);
	sizer_3->Add(label_11, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_3->Add(m_DueMinutesSpinner, 0, 0, 0);
	grid_sizer_2->Add(sizer_3, 1, wxEXPAND, 0);
	grid_sizer_2->Add(label_8, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_2->Add(m_TimezoneChoice, 0, 0, 0);
	grid_sizer_2->Add(label_10, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_5_copy->Add(m_ReminderCheck, 0, 0, 0);
	sizer_5_copy->Add(m_ReminderDateCtrl, 0, 0, 0);
	sizer_5_copy->Add(20, 20, 0, 0, 0);
	sizer_5_copy->Add(m_ReminderHoursSpinner, 0, 0, 5);
	sizer_5_copy->Add(label_6, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_5_copy->Add(m_ReminderMinutesSpinner, 0, wxRIGHT, 5);
	grid_sizer_2->Add(sizer_5_copy, 1, wxEXPAND, 0);
	grid_sizer_2->AddGrowableCol(1);
	sizer_1->Add(grid_sizer_2, 0, wxEXPAND, 0);
	sizer_1->Add(static_line_2, 0, wxALL|wxEXPAND, 5);
	grid_sizer_4->Add(label_18, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_4->Add(m_RecurrenceChoice, 0, 0, 0);
	grid_sizer_4->Add(RecurIntervalLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
	m_IntervalCtrlsSizer->Add(RecurIntervalLabelB, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	m_IntervalCtrlsSizer->Add(m_IntervalSpinner, 0, wxRIGHT, 5);
	m_IntervalCtrlsSizer->Add(m_IntervalUnitLabel, 1, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_4->Add(m_IntervalCtrlsSizer, 1, wxEXPAND, 0);
	grid_sizer_4->Add(RecurDaysLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
	m_DaysCtrlsSizer->Add(m_SunCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_MonCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_TueCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_WedCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_ThuCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_FriCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_SatCheck, 0, wxRIGHT, 5);
	grid_sizer_4->Add(m_DaysCtrlsSizer, 1, wxEXPAND, 0);
	grid_sizer_4->Add(RecurRelativeDateLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_4->Add(m_RelativeDateCheck, 0, 0, 0);
	grid_sizer_4->Add(RecurEndDateLabel, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_8->Add(m_NeverEndsCheck, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10);
	sizer_8->Add(m_RecurEndDateCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_4->Add(sizer_8, 1, wxEXPAND, 0);
	grid_sizer_4->AddGrowableCol(1);
	sizer_1->Add(grid_sizer_4, 0, wxEXPAND, 0);
	sizer_1->Add(static_line_3, 0, wxALL|wxEXPAND, 5);
	grid_sizer_3->Add(label_4, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_3->Add(m_CategoriesText, 0, wxEXPAND, 0);
	grid_sizer_3->Add(label_3, 0, wxRIGHT, 5);
	grid_sizer_3->Add(m_NotesText, 1, wxEXPAND, 0);
	grid_sizer_3->AddGrowableCol(1);
	sizer_1->Add(grid_sizer_3, 1, wxEXPAND, 0);
	sizer_surround->Add(sizer_1, 1, wxALL|wxEXPAND, 10);
	SetSizer(sizer_surround);
	sizer_surround->Fit(this);
	Layout();
	// end wxGlade

	m_top_sizer = sizer_surround;
}

bool TaskEditDlg::TransferDataToWindow()
{
	// prepare temporary variables, from record

	m_rec.Categories.CategoryList2Str(m_categories);

	// due time
	m_DueCheck->SetValue(m_rec.DueTime.IsValid());
	EnableDueDate(m_rec.DueTime.IsValid());
	m_DueDateObj.Set(m_rec.DueTime.Time);

	// alarm / reminder time
	m_ReminderCheck->SetValue(m_rec.AlarmTime.IsValid());
	EnableReminderDate(m_rec.AlarmTime.IsValid());
	m_ReminderDateObj.Set(m_rec.AlarmTime.Time);

	// status
#define S_NOT_STARTED 0
#define S_IN_PROGRESS 1
#define S_COMPLETED 2
#define S_WAITING 3
#define S_DEFERRED 4
	switch( m_rec.StatusFlag )
	{
	case Barry::Task::NotStarted:
	default:
		m_StatusChoice->SetSelection(S_NOT_STARTED);
		break;

	case Barry::Task::InProgress:
		m_StatusChoice->SetSelection(S_IN_PROGRESS);
		break;

	case Barry::Task::Completed:
		m_StatusChoice->SetSelection(S_COMPLETED);
		break;

	case Barry::Task::Waiting:
		m_StatusChoice->SetSelection(S_WAITING);
		break;

	case Barry::Task::Deferred:
		m_StatusChoice->SetSelection(S_DEFERRED);
		break;
	}

	// priority
#define P_HIGH 0
#define P_NORMAL 1
#define P_LOW 2
	switch( m_rec.PriorityFlag )
	{
	case Barry::Task::High:
		m_PriorityChoice->SetSelection(P_HIGH);
		break;

	case Barry::Task::Normal:
	default:
		m_PriorityChoice->SetSelection(P_NORMAL);
		break;

	case Barry::Task::Low:
		m_PriorityChoice->SetSelection(P_LOW);
		break;
	}

	// set the timezone choice only if the record's data is valid
	m_TimezoneChoice->SetSelection(0);	// default to none
	if( m_rec.TimeZoneValid ) {
		TimeZones::const_iterator i = m_zones->Find(m_rec.TimeZoneCode);
		if( i != m_zones->end() ) {
			int array_index = i - m_zones->begin();
			// select item, skipping 0's "none" option
			m_TimezoneChoice->SetSelection(array_index + 1);
		}
	}

	// Note that recur_choice values are (zero-based) in the following
	// order:
	//	None, Daily, Weekly, Monthly, Yearly
#define RC_NONE 0
#define RC_DAILY 1
#define RC_WEEKLY 2
#define RC_MONTHLY 3
#define RC_YEARLY 4

	if( m_rec.Recurring ) {
		switch( m_rec.RecurringType )
		{
		case Barry::RecurBase::Day:
			m_recur_choice = RC_DAILY;
			m_relative_date = false;
			break;

		case Barry::RecurBase::MonthByDate:
			m_recur_choice = RC_MONTHLY;
			m_relative_date = false;
			break;

		case Barry::RecurBase::MonthByDay:
			m_recur_choice = RC_MONTHLY;
			m_relative_date = true;
			break;

		case Barry::RecurBase::YearByDate:
			m_recur_choice = RC_YEARLY;
			m_relative_date = false;
			break;

		case Barry::RecurBase::YearByDay:
			m_recur_choice = RC_YEARLY;
			m_relative_date = true;
			break;

		case Barry::RecurBase::Week:
			m_recur_choice = RC_WEEKLY;
			m_relative_date = false;
			m_weekdays[0] = m_rec.WeekDays & CAL_WD_SUN;
			m_weekdays[1] = m_rec.WeekDays & CAL_WD_MON;
			m_weekdays[2] = m_rec.WeekDays & CAL_WD_TUE;
			m_weekdays[3] = m_rec.WeekDays & CAL_WD_WED;
			m_weekdays[4] = m_rec.WeekDays & CAL_WD_THU;
			m_weekdays[5] = m_rec.WeekDays & CAL_WD_FRI;
			m_weekdays[6] = m_rec.WeekDays & CAL_WD_SAT;
			break;

		default:
			cerr << "Bad RecurringType in CalendarEditDlg" << endl;
			m_recur_choice = RC_NONE;
			m_relative_date = false;
		}
	}
	else {
		m_recur_choice = RC_NONE;
		m_relative_date = false;
	}

	m_interval = m_rec.Interval;

	if( m_rec.Perpetual ) {
		m_RecurEndDateCtrl->Enable(false);
	}
	else {
		m_RecurEndDateCtrl->Enable();
		m_RecurEndDateObj.Set(m_rec.RecurringEndTime.Time);
	}

	EnableRecurMode(m_rec.Recurring);

	m_strings.Refresh();

	return wxDialog::TransferDataToWindow();
}

bool TaskEditDlg::TransferDataFromWindow()
{
	if( !wxDialog::TransferDataFromWindow() )
		return false;

	m_strings.Sync();

	m_rec.Categories.CategoryStr2List(m_categories);

	// due time
	if( m_DueCheck->IsChecked() )
		m_rec.DueTime.Time = m_DueDateObj.Get();
	else
		m_rec.DueTime.clear();

	// alarm / reminder time
	if( m_ReminderCheck->IsChecked() )
		m_rec.AlarmTime.Time = m_ReminderDateObj.Get();
	else
		m_rec.AlarmTime.clear();

	// status
	switch( m_StatusChoice->GetSelection() )
	{
	case S_NOT_STARTED:
	default:
		m_rec.StatusFlag = Barry::Task::NotStarted;
		break;

	case S_IN_PROGRESS:
		m_rec.StatusFlag = Barry::Task::InProgress;
		break;

	case S_COMPLETED:
		m_rec.StatusFlag = Barry::Task::Completed;
		break;

	case S_WAITING:
		m_rec.StatusFlag = Barry::Task::Waiting;
		break;

	case S_DEFERRED:
		m_rec.StatusFlag = Barry::Task::Deferred;
		break;
	}

	// priority
	switch( m_PriorityChoice->GetSelection() )
	{
	case P_HIGH:
		m_rec.PriorityFlag = Barry::Task::High;
		break;

	case P_NORMAL:
	default:
		m_rec.PriorityFlag = Barry::Task::Normal;
		break;

	case P_LOW:
		m_rec.PriorityFlag = Barry::Task::Low;
		break;
	}

	// set the timezone choice only if the record's data is valid
	int sel = m_TimezoneChoice->GetSelection();
	if( sel > 0 ) {
		m_rec.TimeZoneCode = (*m_zones)[sel-1].Index;
		m_rec.TimeZoneValid = true;
	}
	else {
		// default was selected
		m_rec.TimeZoneValid = false;
	}

	// Note that recur_choice values are (zero-based) in the following
	// order:
	//	None, Daily, Weekly, Monthly, Yearly
	switch( m_recur_choice )
	{
	case RC_NONE:
	default:
		m_rec.Recurring = false;
		break;

	case RC_DAILY:
		m_rec.Recurring = true;
		m_rec.RecurringType = Barry::RecurBase::Day;
		break;

	case RC_WEEKLY:
		m_rec.Recurring = true;
		m_rec.RecurringType = Barry::RecurBase::Week;
		m_rec.WeekDays = 0;
		if( m_weekdays[0] ) m_rec.WeekDays |= CAL_WD_SUN;
		if( m_weekdays[1] ) m_rec.WeekDays |= CAL_WD_MON;
		if( m_weekdays[2] ) m_rec.WeekDays |= CAL_WD_TUE;
		if( m_weekdays[3] ) m_rec.WeekDays |= CAL_WD_WED;
		if( m_weekdays[4] ) m_rec.WeekDays |= CAL_WD_THU;
		if( m_weekdays[5] ) m_rec.WeekDays |= CAL_WD_FRI;
		if( m_weekdays[6] ) m_rec.WeekDays |= CAL_WD_SAT;
		break;

	case RC_MONTHLY:
		m_rec.Recurring = true;
		if( m_relative_date )
			m_rec.RecurringType = Barry::RecurBase::MonthByDay;
		else
			m_rec.RecurringType = Barry::RecurBase::MonthByDate;
		break;

	case RC_YEARLY:
		m_rec.Recurring = true;
		if( m_relative_date )
			m_rec.RecurringType = Barry::RecurBase::YearByDay;
		else
			m_rec.RecurringType = Barry::RecurBase::YearByDate;
		break;

	}

	m_rec.Interval = m_interval;

	if( !m_rec.Perpetual ) {
		m_rec.RecurringEndTime.Time = m_RecurEndDateObj.Get();
	}

	return true;
}

void TaskEditDlg::EnableDueDate(bool enable)
{
	m_DueDateCtrl->Enable(enable);
	m_DueHoursSpinner->Enable(enable);
	m_DueMinutesSpinner->Enable(enable);
	m_RecurrenceChoice->Enable(enable);

	if( !enable ) {
		m_RecurrenceChoice->SetSelection(0);
		EnableRecurMode(false);
	}
}

void TaskEditDlg::EnableReminderDate(bool enable)
{
	m_ReminderDateCtrl->Enable(enable);
	m_ReminderHoursSpinner->Enable(enable);
	m_ReminderMinutesSpinner->Enable(enable);
}

void TaskEditDlg::EnableRecurMode(bool recur)
{
	RecurIntervalLabel->Show(recur);
	RecurIntervalLabelB->Show(recur);
	m_IntervalSpinner->Show(recur);
	m_IntervalUnitLabel->Show(recur);
	RecurDaysLabel->Show(recur);
	m_SunCheck->Show(recur);
	m_MonCheck->Show(recur);
	m_TueCheck->Show(recur);
	m_WedCheck->Show(recur);
	m_ThuCheck->Show(recur);
	m_FriCheck->Show(recur);
	m_SatCheck->Show(recur);
	RecurRelativeDateLabel->Show(recur);
	m_RelativeDateCheck->Show(recur);
	RecurEndDateLabel->Show(recur);
	m_NeverEndsCheck->Show(recur);
	m_RecurEndDateCtrl->Show(recur);

	if( recur ) {
		switch( m_RecurrenceChoice->GetSelection() )
		{
		case RC_NONE:
		default:
			m_IntervalUnitLabel->SetLabel(_T(""));
			break;

		case RC_DAILY:
			m_IntervalUnitLabel->SetLabel(_T("day(s)"));
			break;

		case RC_WEEKLY:
			m_IntervalUnitLabel->SetLabel(_T("week(s)"));
			break;

		case RC_MONTHLY:
			m_IntervalUnitLabel->SetLabel(_T("month(s)"));
			break;

		case RC_YEARLY:
			m_IntervalUnitLabel->SetLabel(_T("year(s)"));
			break;
		}
	}

	RedoLayout();
}

