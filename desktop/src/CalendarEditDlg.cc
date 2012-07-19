///
/// \file	CalendarEditDlg.cc
///		Dialog class to handle the editing of the Calendar record
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

#include "CalendarEditDlg.h"
#include "windowids.h"
#include <wx/valgen.h>
#include "wxval.h"
#include "util.h"

using namespace std;
using namespace Barry;

// begin wxGlade: ::extracode
// end wxGlade


//////////////////////////////////////////////////////////////////////////////
// CalendarEditDlg class

CalendarEditDlg::CalendarEditDlg(wxWindow* parent,
				Barry::Calendar &rec,
				bool editable,
				const Barry::TimeZones *device_zones)
	: wxDialog(parent, Dialog_CalendarEdit, _W("Calendar Record"))
	, m_zones(device_zones ? device_zones : &m_static_zones)
	, m_rec(rec)
	, m_duration_hours(0)
	, m_duration_minutes(0)
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

	// begin wxGlade: CalendarEditDlg::CalendarEditDlg
	label_1 = new wxStaticText(this, wxID_ANY, _W("Subject:"));
	m_Subject = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_2 = new wxStaticText(this, wxID_ANY, _W("Location:"));
	m_Location = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_1 = new wxStaticLine(this, wxID_ANY);
	label_4 = new wxStaticText(this, wxID_ANY, _W("All Day Event:"));
	m_AllDayCheck = new wxCheckBox(this, Dialog_CalendarEdit_AllDayCheck, wxEmptyString);
	label_5 = new wxStaticText(this, wxID_ANY, _W("Start:"));
	m_StartDateCtrl = new wxDatePickerCtrl(this, Dialog_CalendarEdit_StartDateCtrl, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	m_StartHoursSpinner = new wxSpinCtrl(this, Dialog_CalendarEdit_StartHoursSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 23);
	label_11 = new wxStaticText(this, wxID_ANY, wxT(":"));
	m_StartMinutesSpinner = new wxSpinCtrl(this, Dialog_CalendarEdit_StartMinutesSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 59);
	label_6 = new wxStaticText(this, wxID_ANY, _W("End:"));
	m_EndDateCtrl = new wxDatePickerCtrl(this, Dialog_CalendarEdit_EndDateCtrl, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	m_EndHoursSpinner = new wxSpinCtrl(this, Dialog_CalendarEdit_EndHoursSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 23);
	label_12 = new wxStaticText(this, wxID_ANY, wxT(":"));
	m_EndMinutesSpinner = new wxSpinCtrl(this, Dialog_CalendarEdit_EndMinutesSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 59);
	label_7 = new wxStaticText(this, wxID_ANY, _W("Duration:"));
	m_DurationHoursSpinner = new wxSpinCtrl(this, Dialog_CalendarEdit_DurationHoursSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999);
	label_13 = new wxStaticText(this, wxID_ANY, _W("hours and"));
	m_DurationMinutesSpinner = new wxSpinCtrl(this, Dialog_CalendarEdit_DurationMinutesSpinner, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59);
	label_17 = new wxStaticText(this, wxID_ANY, _W("minutes."));
	label_8 = new wxStaticText(this, wxID_ANY, _W("Time Zone:"));
	const wxString m_TimezoneChoice_choices[] = {
        _W("System Time Zone")
    };
	m_TimezoneChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 1, m_TimezoneChoice_choices, 0);
	label_9 = new wxStaticText(this, wxID_ANY, _W("Show As:"));
/*
	const wxString m_ShowAsChoice_choices[] = {
        wxT("Free"),
        wxT("Tentative"),
        wxT("Busy"),
        wxT("Out of Office")
    };
*/
wxArrayString m_ShowAsChoice_choices;
m_ShowAsChoice_choices.Add( _W("Free") );
m_ShowAsChoice_choices.Add( _W("Tentative") );
m_ShowAsChoice_choices.Add( _W("Busy") );
m_ShowAsChoice_choices.Add( _W("Out of Office") );

	m_ShowAsChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ShowAsChoice_choices, 0);
	label_10 = new wxStaticText(this, wxID_ANY, _W("Reminder:"));
	m_ReminderHoursSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999);
	label_13_copy = new wxStaticText(this, wxID_ANY, _W("hours and"));
	m_ReminderMinutesSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59);
	label_17_copy = new wxStaticText(this, wxID_ANY, _W("minutes."));
	static_line_2 = new wxStaticLine(this, wxID_ANY);
	label_18 = new wxStaticText(this, wxID_ANY, _W("Recurrence:"));
/*
	const wxString m_RecurrenceChoice_choices[] = {
        wxT("None"),
        wxT("Daily"),
        wxT("Weekly"),
        wxT("Monthly"),
        wxT("Yearly")
    };
*/
wxArrayString m_RecurrenceChoice_choices;
m_RecurrenceChoice_choices.Add( _W("None") );
m_RecurrenceChoice_choices.Add( _W("Daily") );
m_RecurrenceChoice_choices.Add( _W("Weekly") );
m_RecurrenceChoice_choices.Add( _W("Monthly") );
m_RecurrenceChoice_choices.Add( _W("Yearly") );

	m_RecurrenceChoice = new wxChoice(this, Dialog_CalendarEdit_RecurrenceChoice, wxDefaultPosition, wxDefaultSize, m_RecurrenceChoice_choices, 0);
	RecurIntervalLabel = new wxStaticText(this, wxID_ANY, _W("Interval:"));
	RecurIntervalLabelB = new wxStaticText(this, wxID_ANY, _W("Every"));
	m_IntervalSpinner = new wxSpinCtrl(this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999);
	m_IntervalUnitLabel = new wxStaticText(this, wxID_ANY, _W("days? weeks? months?"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	RecurDaysLabel = new wxStaticText(this, wxID_ANY, _W("Days:"));
	m_SunCheck = new wxCheckBox(this, wxID_ANY, wxT("S"));
	m_MonCheck = new wxCheckBox(this, wxID_ANY, wxT("M"));
	m_TueCheck = new wxCheckBox(this, wxID_ANY, wxT("T"));
	m_WedCheck = new wxCheckBox(this, wxID_ANY, wxT("W"));
	m_ThuCheck = new wxCheckBox(this, wxID_ANY, wxT("T"));
	m_FriCheck = new wxCheckBox(this, wxID_ANY, wxT("F"));
	m_SatCheck = new wxCheckBox(this, wxID_ANY, wxT("S"));
	RecurRelativeDateLabel = new wxStaticText(this, wxID_ANY, _W("Relative Date:"));
	m_RelativeDateCheck = new wxCheckBox(this, wxID_ANY, wxEmptyString);
	RecurEndDateLabel = new wxStaticText(this, wxID_ANY, _W("End Date:"));
	m_NeverEndsCheck = new wxCheckBox(this, Dialog_CalendarEdit_NeverEndsCheck, _W("Never ends"));
	m_RecurEndDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	static_line_3 = new wxStaticLine(this, wxID_ANY);
	label_14 = new wxStaticText(this, wxID_ANY, _W("Organizer:"));
	m_OrganizerText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_15 = new wxStaticText(this, wxID_ANY, _W("Invited:"));
	m_InvitedText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_16 = new wxStaticText(this, wxID_ANY, _W("Accepted By:"));
	m_AcceptedByText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_4 = new wxStaticLine(this, wxID_ANY);
/*
	const wxString m_ClassRadioBox_choices[] = {
        wxT("Public"),
        wxT("Private"),
        wxT("Confidential")
    };
*/
wxArrayString m_ClassRadioBox_choices;
m_ClassRadioBox_choices.Add( _W("Public") );
m_ClassRadioBox_choices.Add( _W("Private") );
m_ClassRadioBox_choices.Add( _W("Confidential") );

	m_ClassRadioBox = new wxRadioBox(this, wxID_ANY, _W("Class"), wxDefaultPosition, wxDefaultSize, m_ClassRadioBox_choices, 3, wxRA_SPECIFY_COLS);
	static_line_5 = new wxStaticLine(this, wxID_ANY);
	label_3 = new wxStaticText(this, wxID_ANY, _W("Notes:"));
	m_NotesText = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

	set_properties();
	do_layout();
	// end wxGlade

	m_top_sizer->Add(bottom_buttons, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);

	// fill the time zone control with real time zones
	m_TimezoneChoice->Clear();
	m_TimezoneChoice->Append(_W("Assume Local Timezone"), (void*)0);
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

void CalendarEditDlg::RedoLayout()
{
	m_top_sizer->Fit(this);
	Layout();
}

BEGIN_EVENT_TABLE(CalendarEditDlg, wxDialog)
	// begin wxGlade: CalendarEditDlg::event_table
	EVT_CHECKBOX(Dialog_CalendarEdit_AllDayCheck, CalendarEditDlg::OnAllDayEvent)
	EVT_DATE_CHANGED(Dialog_CalendarEdit_StartDateCtrl, CalendarEditDlg::OnStartDateChanged)
	EVT_SPINCTRL(Dialog_CalendarEdit_StartHoursSpinner, CalendarEditDlg::OnStartHoursSpin)
	EVT_SPINCTRL(Dialog_CalendarEdit_StartMinutesSpinner, CalendarEditDlg::OnStartMinutesSpin)
	EVT_DATE_CHANGED(Dialog_CalendarEdit_EndDateCtrl, CalendarEditDlg::OnEndDateChanged)
	EVT_SPINCTRL(Dialog_CalendarEdit_EndHoursSpinner, CalendarEditDlg::OnEndHoursSpin)
	EVT_SPINCTRL(Dialog_CalendarEdit_EndMinutesSpinner, CalendarEditDlg::OnEndMinutesSpin)
	EVT_SPINCTRL(Dialog_CalendarEdit_DurationHoursSpinner, CalendarEditDlg::OnDurationHoursSpin)
	EVT_SPINCTRL(Dialog_CalendarEdit_DurationMinutesSpinner, CalendarEditDlg::OnDurationMinutesSpin)
	EVT_CHOICE(Dialog_CalendarEdit_RecurrenceChoice, CalendarEditDlg::OnRecurrenceChoice)
	EVT_CHECKBOX(Dialog_CalendarEdit_NeverEndsCheck, CalendarEditDlg::OnEndDateCheckbox)
	// end wxGlade
END_EVENT_TABLE();


void CalendarEditDlg::OnAllDayEvent(wxCommandEvent &event)
{
	bool checked = m_AllDayCheck->IsChecked();

	if( checked ) {
		// set start time to date at 00:00 and end time at
		// day + 1 at 00:00

		// time
		m_StartHoursSpinner->SetValue(0);
		m_StartMinutesSpinner->SetValue(0);
		m_EndHoursSpinner->SetValue(0);
		m_EndMinutesSpinner->SetValue(0);

		// date
		m_StartDateCtrl->SetValue(m_StartDateCtrl->GetValue().GetDateOnly());
		m_EndDateCtrl->SetValue(m_StartDateCtrl->GetValue().GetDateOnly() + wxDateSpan::Day());

		// duration
		m_DurationHoursSpinner->SetValue(24);
		m_DurationMinutesSpinner->SetValue(0);
	}

	EnableAllDayMode(checked);
}

void CalendarEditDlg::OnStartDateChanged(wxDateEvent &event)
{
	UpdateDuration();
}

void CalendarEditDlg::OnStartHoursSpin(wxSpinEvent &event)
{
	UpdateDuration();
}

void CalendarEditDlg::OnStartMinutesSpin(wxSpinEvent &event)
{
	UpdateDuration();
}

void CalendarEditDlg::OnEndDateChanged(wxDateEvent &event)
{
	UpdateDuration();
}

void CalendarEditDlg::OnEndHoursSpin(wxSpinEvent &event)
{
	UpdateDuration();
}

void CalendarEditDlg::OnEndMinutesSpin(wxSpinEvent &event)
{
	UpdateDuration();
}

void CalendarEditDlg::OnDurationHoursSpin(wxSpinEvent &event)
{
	UpdateEndDate();
}

void CalendarEditDlg::OnDurationMinutesSpin(wxSpinEvent &event)
{
	UpdateEndDate();
}

void CalendarEditDlg::OnRecurrenceChoice(wxCommandEvent &event)
{
	TransferDataFromWindow();
	EnableRecurMode(m_rec.Recurring);
}


void CalendarEditDlg::OnEndDateCheckbox(wxCommandEvent &event)
{
	m_RecurEndDateCtrl->Enable( !m_NeverEndsCheck->IsChecked() );
}


// wxGlade: add CalendarEditDlg event handlers


void CalendarEditDlg::set_properties()
{
	// begin wxGlade: CalendarEditDlg::set_properties
	SetTitle(_W("Calendar Event"));
	m_Subject->SetFocus();
	m_Subject->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Subject)));
	m_Location->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Location)));
	m_AllDayCheck->SetValidator(wxGenericValidator(&m_rec.AllDayEvent));
	m_StartDateCtrl->SetMinSize(wxSize(110, -1));
	m_StartDateCtrl->SetValidator(DateTimeValidator(&m_StartDateObj.m_date));
	m_StartHoursSpinner->SetMinSize(wxSize(45, -1));
	m_StartHoursSpinner->SetValidator(wxGenericValidator(&m_StartDateObj.m_hour));
	m_StartMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_StartMinutesSpinner->SetValidator(wxGenericValidator(&m_StartDateObj.m_min));
	m_EndDateCtrl->SetMinSize(wxSize(110, -1));
	m_EndDateCtrl->SetValidator(DateTimeValidator(&m_EndDateObj.m_date));
	m_EndHoursSpinner->SetMinSize(wxSize(45, -1));
	m_EndHoursSpinner->SetValidator(wxGenericValidator(&m_EndDateObj.m_hour));
	m_EndMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_EndMinutesSpinner->SetValidator(wxGenericValidator(&m_EndDateObj.m_min));
	m_DurationHoursSpinner->SetMinSize(wxSize(45, -1));
	m_DurationHoursSpinner->SetValidator(wxGenericValidator(&m_duration_hours));
	m_DurationMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_DurationMinutesSpinner->SetValidator(wxGenericValidator(&m_duration_minutes));
	m_TimezoneChoice->SetSelection(0);
	m_ShowAsChoice->SetSelection(2);
	m_ReminderHoursSpinner->SetMinSize(wxSize(45, -1));
	m_ReminderHoursSpinner->SetToolTip(_W("Set Reminder to 0 to disable"));
	m_ReminderHoursSpinner->SetValidator(wxGenericValidator(&m_reminder_hours));
	m_ReminderMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_ReminderMinutesSpinner->SetToolTip(_W("Set Reminder to 0 to disable"));
	m_ReminderMinutesSpinner->SetValidator(wxGenericValidator(&m_reminder_minutes));
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
	RecurRelativeDateLabel->SetToolTip(_W("Relative monthly or yearly dates take the weekday of the start date into account. (eg. every first Sunday of month)"));
	m_RelativeDateCheck->SetToolTip(_W("Relative monthly or yearly dates take the weekday of the start date into account. (eg. every first Sunday of month)"));
	m_RelativeDateCheck->SetValidator(wxGenericValidator(&m_relative_date));
	m_NeverEndsCheck->SetValidator(wxGenericValidator(&m_rec.Perpetual));
	m_NeverEndsCheck->SetValue(1);
	m_RecurEndDateCtrl->SetMinSize(wxSize(110, -1));
	m_RecurEndDateCtrl->Enable(false);
	m_RecurEndDateCtrl->SetValidator(DateTimeValidator(&m_RecurEndDateObj.m_date));
	m_OrganizerText->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_organizer)));
	m_InvitedText->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_invited)));
	m_AcceptedByText->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_accepted_by)));
	m_ClassRadioBox->SetValidator(MakeRadioBoxValidator(&m_rec.ClassFlag).Add(Barry::Calendar::Public).Add(Barry::Calendar::Private).Add(Barry::Calendar::Confidential));
	m_ClassRadioBox->SetSelection(0);
	m_NotesText->SetMinSize(wxSize(-1, 61));
	m_NotesText->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Notes)));
	// end wxGlade
}


void CalendarEditDlg::do_layout()
{
	// begin wxGlade: CalendarEditDlg::do_layout
	wxBoxSizer* sizer_surround = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_3 = new wxFlexGridSizer(3, 2, 5, 5);
	wxFlexGridSizer* grid_sizer_4 = new wxFlexGridSizer(5, 2, 5, 5);
	wxBoxSizer* sizer_8 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* m_DaysCtrlsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* m_IntervalCtrlsSizer = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_2 = new wxFlexGridSizer(7, 2, 5, 5);
	wxBoxSizer* sizer_5_copy = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_5 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_1 = new wxFlexGridSizer(2, 2, 5, 5);
	grid_sizer_1->Add(label_1, 0, 0, 0);
	grid_sizer_1->Add(m_Subject, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_2, 0, 0, 0);
	grid_sizer_1->Add(m_Location, 0, wxEXPAND, 0);
	grid_sizer_1->AddGrowableCol(1);
	sizer_1->Add(grid_sizer_1, 0, wxEXPAND, 0);
	sizer_1->Add(static_line_1, 0, wxALL|wxEXPAND, 5);
	grid_sizer_2->Add(label_4, 0, 0, 0);
	grid_sizer_2->Add(m_AllDayCheck, 0, 0, 0);
	grid_sizer_2->Add(label_5, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_3->Add(m_StartDateCtrl, 0, 0, 0);
	sizer_3->Add(20, 20, 0, 0, 0);
	sizer_3->Add(m_StartHoursSpinner, 0, 0, 0);
	sizer_3->Add(label_11, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_3->Add(m_StartMinutesSpinner, 0, 0, 0);
	grid_sizer_2->Add(sizer_3, 1, wxEXPAND, 0);
	grid_sizer_2->Add(label_6, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_4->Add(m_EndDateCtrl, 0, 0, 0);
	sizer_4->Add(20, 20, 0, 0, 0);
	sizer_4->Add(m_EndHoursSpinner, 0, 0, 0);
	sizer_4->Add(label_12, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1);
	sizer_4->Add(m_EndMinutesSpinner, 0, 0, 0);
	grid_sizer_2->Add(sizer_4, 1, wxEXPAND, 0);
	grid_sizer_2->Add(label_7, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_5->Add(m_DurationHoursSpinner, 0, wxRIGHT, 5);
	sizer_5->Add(label_13, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	sizer_5->Add(m_DurationMinutesSpinner, 0, wxRIGHT, 5);
	sizer_5->Add(label_17, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_2->Add(sizer_5, 1, wxEXPAND, 0);
	grid_sizer_2->Add(label_8, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_2->Add(m_TimezoneChoice, 0, 0, 0);
	grid_sizer_2->Add(label_9, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_2->Add(m_ShowAsChoice, 0, 0, 0);
	grid_sizer_2->Add(label_10, 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_5_copy->Add(m_ReminderHoursSpinner, 0, wxRIGHT, 5);
	sizer_5_copy->Add(label_13_copy, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	sizer_5_copy->Add(m_ReminderMinutesSpinner, 0, wxRIGHT, 5);
	sizer_5_copy->Add(label_17_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
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
	grid_sizer_3->Add(label_14, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_3->Add(m_OrganizerText, 0, wxEXPAND, 0);
	grid_sizer_3->Add(label_15, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_3->Add(m_InvitedText, 0, wxEXPAND, 0);
	grid_sizer_3->Add(label_16, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_3->Add(m_AcceptedByText, 0, wxEXPAND, 0);
	grid_sizer_3->AddGrowableCol(1);
	sizer_1->Add(grid_sizer_3, 0, wxEXPAND, 0);
	sizer_1->Add(static_line_4, 0, wxALL|wxEXPAND, 5);
	sizer_1->Add(m_ClassRadioBox, 0, wxEXPAND, 0);
	sizer_1->Add(static_line_5, 0, wxALL|wxEXPAND, 5);
	sizer_2->Add(label_3, 0, wxRIGHT, 5);
	sizer_2->Add(m_NotesText, 1, wxEXPAND, 0);
	sizer_1->Add(sizer_2, 1, wxEXPAND, 0);
	sizer_surround->Add(sizer_1, 1, wxALL|wxEXPAND, 10);
	SetSizer(sizer_surround);
	sizer_surround->Fit(this);
	Layout();
	// end wxGlade

	m_top_sizer = sizer_surround;
}

bool CalendarEditDlg::TransferDataToWindow()
{
	// prepare temporary variables, from record

	m_organizer = m_rec.Organizer.ToCommaSeparated();
	m_invited = m_rec.Invited.ToCommaSeparated();
	m_accepted_by = m_rec.AcceptedBy.ToCommaSeparated();

	m_StartDateObj.Set(m_rec.StartTime.Time);
	m_EndDateObj.Set(m_rec.EndTime.Time);

	int duration = m_rec.EndTime.Time - m_rec.StartTime.Time;
	duration /= 60;			// convert to minutes
	if( m_rec.EndTime.Time >= m_rec.StartTime.Time ) {
		m_duration_hours = duration / 60;
		m_duration_minutes = duration % 60;
	}

	if( m_rec.NotificationTime.Time ) {
		int span = m_rec.StartTime.Time - m_rec.NotificationTime.Time;
		span /= 60;		// convert to minutes
		if( m_rec.StartTime.Time > m_rec.NotificationTime.Time ) {
			m_reminder_hours = span / 60;
			m_reminder_minutes = span % 60;
		}
		else {
			m_reminder_hours = 0;
			m_reminder_minutes = 15;
		}
	}

	EnableAllDayMode(m_rec.AllDayEvent);

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

#define SA_FREE 0
#define SA_TENTATIVE 1
#define SA_BUSY 2
#define SA_OUT_OF_OFFICE 3
	switch( m_rec.FreeBusyFlag )
	{
	case Barry::Calendar::Free:
		m_ShowAsChoice->SetSelection(SA_FREE);
		break;

	case Barry::Calendar::Tentative:
		m_ShowAsChoice->SetSelection(SA_TENTATIVE);
		break;

	case Barry::Calendar::Busy:
	default:
		m_ShowAsChoice->SetSelection(SA_BUSY);
		break;

	case Barry::Calendar::OutOfOffice:
		m_ShowAsChoice->SetSelection(SA_OUT_OF_OFFICE);
		break;
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

	m_RecurEndDateObj.Set(m_rec.RecurringEndTime.Time);
	if( m_rec.Perpetual ) {
		m_RecurEndDateCtrl->Enable(false);
	}
	else {
		m_RecurEndDateCtrl->Enable();
	}

	EnableRecurMode(m_rec.Recurring);

	m_strings.RefreshWx();

	// let the base class call the validaors to do the rest
	if( wxDialog::TransferDataToWindow() ) {
		// on success, do just a little bit of fine tuning
		MakeDateRecent(true, m_StartDateCtrl);
		MakeDateRecent(true, m_EndDateCtrl);
		MakeDateRecent(true, m_RecurEndDateCtrl);
		return true;
	}
	else {
		return false;
	}
}

bool CalendarEditDlg::TransferDataFromWindow()
{
	if( !wxDialog::TransferDataFromWindow() )
		return false;

	m_strings.Sync();

	m_rec.Organizer.clear();
	m_rec.Organizer.AddCommaSeparated(m_organizer);

	m_rec.Invited.clear();
	m_rec.Invited.AddCommaSeparated(m_invited);

	m_rec.AcceptedBy.clear();
	m_rec.AcceptedBy.AddCommaSeparated(m_accepted_by);

	m_rec.StartTime.Time = m_StartDateObj.Get();
	m_rec.EndTime.Time = m_EndDateObj.Get();
	if( m_rec.EndTime.Time < m_rec.StartTime.Time ) {
		wxMessageBox(_W("Start time must come before end time."),
			_W("Validation Error"), wxOK | wxICON_INFORMATION);
		return false;
	}

	int span = ((m_reminder_hours * 60) + m_reminder_minutes) * 60;
	m_rec.NotificationTime.Time = m_rec.StartTime.Time - span;

	switch( m_ShowAsChoice->GetSelection() )
	{
	case SA_FREE:
		m_rec.FreeBusyFlag = Barry::Calendar::Free;
		break;

	case SA_TENTATIVE:
		m_rec.FreeBusyFlag = Barry::Calendar::Tentative;
		break;

	case SA_BUSY:
	default:
		m_rec.FreeBusyFlag = Barry::Calendar::Busy;
		break;

	case SA_OUT_OF_OFFICE:
		m_rec.FreeBusyFlag = Barry::Calendar::OutOfOffice;
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

void CalendarEditDlg::UpdateDuration()
{
	TransferDataFromWindow();

	if( m_EndDateObj.Get() < m_StartDateObj.Get() ) {
		// error... end date cannot be before start date
		// set end date to start date
		m_EndDateCtrl->SetValue(m_StartDateCtrl->GetValue());
		m_EndHoursSpinner->SetValue(m_StartHoursSpinner->GetValue());
		m_EndMinutesSpinner->SetValue(m_StartMinutesSpinner->GetValue());

		// duration is 0
		m_DurationHoursSpinner->SetValue(0);
		m_DurationMinutesSpinner->SetValue(0);
		return;
	}
	else {
		// set duration to time span of End - Start
		time_t diff = m_EndDateObj.Get() - m_StartDateObj.Get();
		diff /= 60;		// convert to minutes
		m_DurationHoursSpinner->SetValue(diff / 60);
		m_DurationMinutesSpinner->SetValue(diff % 60);
	}
}

void CalendarEditDlg::UpdateEndDate()
{
	TransferDataFromWindow();

	m_EndDateObj.Set( m_StartDateObj.Get() +
		m_duration_hours * 60 * 60 +
		m_duration_minutes * 60 );

	m_EndDateCtrl->SetValue(wxDateTime(m_EndDateObj.m_date));
	m_EndHoursSpinner->SetValue(m_EndDateObj.m_hour);
	m_EndMinutesSpinner->SetValue(m_EndDateObj.m_min);
}

void CalendarEditDlg::EnableAllDayMode(bool all_day)
{
	// if in all day mode, disable start time, end time, duration spinners
	m_StartHoursSpinner->Enable(!all_day);
	m_StartMinutesSpinner->Enable(!all_day);
	m_EndHoursSpinner->Enable(!all_day);
	m_EndMinutesSpinner->Enable(!all_day);
	m_DurationHoursSpinner->Enable(!all_day);
	m_DurationMinutesSpinner->Enable(!all_day);
}

void CalendarEditDlg::EnableRecurMode(bool recur)
{
	// show all controls
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

	// enable based on choice
	int choice = m_RecurrenceChoice->GetSelection();
	m_SunCheck->Enable(choice == RC_WEEKLY);
	m_MonCheck->Enable(choice == RC_WEEKLY);
	m_TueCheck->Enable(choice == RC_WEEKLY);
	m_WedCheck->Enable(choice == RC_WEEKLY);
	m_ThuCheck->Enable(choice == RC_WEEKLY);
	m_FriCheck->Enable(choice == RC_WEEKLY);
	m_SatCheck->Enable(choice == RC_WEEKLY);

	// update labels
	if( recur ) {
		switch( m_RecurrenceChoice->GetSelection() )
		{
		case RC_NONE:
		default:
			m_IntervalUnitLabel->SetLabel(_T(""));
			break;

		case RC_DAILY:
			m_IntervalUnitLabel->SetLabel(_W("day(s)"));
			break;

		case RC_WEEKLY:
			m_IntervalUnitLabel->SetLabel(_W("week(s)"));
			break;

		case RC_MONTHLY:
			m_IntervalUnitLabel->SetLabel(_W("month(s)"));
			break;

		case RC_YEARLY:
			m_IntervalUnitLabel->SetLabel(_W("year(s)"));
			break;
		}
	}

	RedoLayout();
}


//
// Note: this file is very similar to TaskEditDlg.cc, and should be kept
// in lock step as much as possible.  They are in separate files, since
// the GUI code is generated with wxglade
//

