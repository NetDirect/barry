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
	: wxDialog(parent, Dialog_CalendarEdit, _T("Calendar Record"))
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
	label_1 = new wxStaticText(this, wxID_ANY, wxT("Subject:"));
	m_Subject = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_2 = new wxStaticText(this, wxID_ANY, wxT("Location:"));
	m_Location = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_1 = new wxStaticLine(this, wxID_ANY);
	label_4 = new wxStaticText(this, wxID_ANY, wxT("All Day Event:"));
	m_AllDayCheck = new wxCheckBox(this, wxID_ANY, wxEmptyString);
	label_5 = new wxStaticText(this, wxID_ANY, wxT("Start:"));
	m_StartDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	m_StartHoursSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 23);
	label_11 = new wxStaticText(this, wxID_ANY, wxT(":"));
	m_StartMinutesSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 59);
	label_6 = new wxStaticText(this, wxID_ANY, wxT("End:"));
	m_EndDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	m_EndHoursSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 23);
	label_12 = new wxStaticText(this, wxID_ANY, wxT(":"));
	m_EndMinutesSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_WRAP|wxTE_NOHIDESEL, 0, 59);
	label_7 = new wxStaticText(this, wxID_ANY, wxT("Duration:"));
	m_DurationHoursSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999);
	label_13 = new wxStaticText(this, wxID_ANY, wxT("hours and"));
	m_DurationMinutesSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59);
	label_17 = new wxStaticText(this, wxID_ANY, wxT("minutes."));
	label_8 = new wxStaticText(this, wxID_ANY, wxT("Time Zone:"));
	const wxString m_TimezoneChoice_choices[] = {
        wxT("System Time Zone")
    };
	m_TimezoneChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 1, m_TimezoneChoice_choices, 0);
	label_9 = new wxStaticText(this, wxID_ANY, wxT("Show As:"));
	const wxString m_ShowAsChoice_choices[] = {
        wxT("Free"),
        wxT("Tentative"),
        wxT("Busy"),
        wxT("Out of Office")
    };
	m_ShowAsChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 4, m_ShowAsChoice_choices, 0);
	label_10 = new wxStaticText(this, wxID_ANY, wxT("Reminder:"));
	m_ReminderHoursSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999);
	label_13_copy = new wxStaticText(this, wxID_ANY, wxT("hours and"));
	m_ReminderMinutesSpinner = new wxSpinCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59);
	label_17_copy = new wxStaticText(this, wxID_ANY, wxT("minutes."));
	static_line_2 = new wxStaticLine(this, wxID_ANY);
	label_18 = new wxStaticText(this, wxID_ANY, wxT("Recurrence:"));
	const wxString m_RecurrenceChoice_choices[] = {
        wxT("None"),
        wxT("Daily"),
        wxT("Weekly"),
        wxT("Monthly"),
        wxT("Yearly")
    };
	m_RecurrenceChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 5, m_RecurrenceChoice_choices, 0);
	label_19 = new wxStaticText(this, wxID_ANY, wxT("Interval:"));
	label_23 = new wxStaticText(this, wxID_ANY, wxT("Every"));
	m_IntervalSpinner = new wxSpinCtrl(this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999);
	m_IntervalUnitLabel = new wxStaticText(this, wxID_ANY, wxT("days? weeks? months?"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	label_20 = new wxStaticText(this, wxID_ANY, wxT("Days:"));
	m_SunCheck = new wxCheckBox(this, wxID_ANY, wxT("S"));
	m_MonCheck = new wxCheckBox(this, wxID_ANY, wxT("M"));
	m_TueCheck = new wxCheckBox(this, wxID_ANY, wxT("T"));
	m_WedCheck = new wxCheckBox(this, wxID_ANY, wxT("W"));
	m_ThuCheck = new wxCheckBox(this, wxID_ANY, wxT("T"));
	m_FriCheck = new wxCheckBox(this, wxID_ANY, wxT("F"));
	m_SatCheck = new wxCheckBox(this, wxID_ANY, wxT("S"));
	label_21 = new wxStaticText(this, wxID_ANY, wxT("Relative Date:"));
	m_RelativeDateCheck = new wxCheckBox(this, wxID_ANY, wxEmptyString);
	label_22 = new wxStaticText(this, wxID_ANY, wxT("End Date:"));
	m_NeverEndsCheck = new wxCheckBox(this, wxID_ANY, wxT("Never ends"));
	m_RecurEndDateCtrl = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN|wxDP_SHOWCENTURY);
	static_line_3 = new wxStaticLine(this, wxID_ANY);
	label_14 = new wxStaticText(this, wxID_ANY, wxT("Organizer:"));
	m_OrganizerText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_15 = new wxStaticText(this, wxID_ANY, wxT("Invited:"));
	m_InvitedText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_16 = new wxStaticText(this, wxID_ANY, wxT("Accepted By:"));
	m_AcceptedByText = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_4 = new wxStaticLine(this, wxID_ANY);
	const wxString m_ClassRadioBox_choices[] = {
        wxT("Public"),
        wxT("Private"),
        wxT("Confidential")
    };
	m_ClassRadioBox = new wxRadioBox(this, wxID_ANY, wxT("Class"), wxDefaultPosition, wxDefaultSize, 3, m_ClassRadioBox_choices, 3, wxRA_SPECIFY_COLS);
	static_line_5 = new wxStaticLine(this, wxID_ANY);
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

void CalendarEditDlg::RedoLayout()
{
	m_top_sizer->Fit(this);
	Layout();
}

BEGIN_EVENT_TABLE(CalendarEditDlg, wxDialog)
	// begin wxGlade: CalendarEditDlg::event_table
	EVT_CHECKBOX(wxID_ANY, CalendarEditDlg::OnAllDayEvent)
	EVT_DATE_CHANGED(wxID_ANY, CalendarEditDlg::OnStartDateChanged)
	EVT_SPINCTRL(wxID_ANY, CalendarEditDlg::OnStartHoursSpin)
	EVT_SPINCTRL(wxID_ANY, CalendarEditDlg::OnStartMinutesSpin)
	EVT_DATE_CHANGED(wxID_ANY, CalendarEditDlg::OnEndDateChanged)
	EVT_SPINCTRL(wxID_ANY, CalendarEditDlg::OnEndHoursSpin)
	EVT_SPINCTRL(wxID_ANY, CalendarEditDlg::OnEndMinutesSpin)
	EVT_SPINCTRL(wxID_ANY, CalendarEditDlg::OnDurationHoursSpin)
	EVT_SPINCTRL(wxID_ANY, CalendarEditDlg::OnDurationMinutesSpin)
	EVT_CHOICE(wxID_ANY, CalendarEditDlg::OnRecurrenceChoice)
	EVT_CHECKBOX(wxID_ANY, CalendarEditDlg::OnEndDateCheckbox)
	// end wxGlade
END_EVENT_TABLE();


void CalendarEditDlg::OnAllDayEvent(wxCommandEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnAllDayEvent) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnStartDateChanged(wxDateEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnStartDateChanged) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnStartHoursSpin(wxSpinEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnStartHoursSpin) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnStartMinutesSpin(wxSpinEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnStartMinutesSpin) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnEndDateChanged(wxDateEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnEndDateChanged) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnEndHoursSpin(wxSpinEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnEndHoursSpin) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnEndMinutesSpin(wxSpinEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnEndMinutesSpin) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnDurationHoursSpin(wxSpinEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnDurationHoursSpin) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnDurationMinutesSpin(wxSpinEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnDurationMinutesSpin) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnRecurrenceChoice(wxCommandEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnRecurrenceChoice) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


void CalendarEditDlg::OnEndDateCheckbox(wxCommandEvent &event)
{
	event.Skip();
	wxLogDebug(wxT("Event handler (CalendarEditDlg::OnEndDateCheckbox) not implemented yet")); //notify the user that he hasn't implemented the event handler yet
}


// wxGlade: add CalendarEditDlg event handlers


void CalendarEditDlg::set_properties()
{
	// begin wxGlade: CalendarEditDlg::set_properties
	SetTitle(wxT("dialog_1"));
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
	m_ReminderHoursSpinner->SetToolTip(wxT("Set Reminder to 0 to disable"));
	m_ReminderHoursSpinner->SetValidator(wxGenericValidator(&m_reminder_hours));
	m_ReminderMinutesSpinner->SetMinSize(wxSize(45, -1));
	m_ReminderMinutesSpinner->SetToolTip(wxT("Set Reminder to 0 to disable"));
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
	label_21->SetToolTip(wxT("Relative monthly or yearly dates take the weekday of the start date into account. (eg. every first Sunday of month)"));
	m_RelativeDateCheck->SetToolTip(wxT("Relative monthly or yearly dates take the weekday of the start date into account. (eg. every first Sunday of month)"));
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
	grid_sizer_4->Add(label_19, 0, wxALIGN_CENTER_VERTICAL, 0);
	m_IntervalCtrlsSizer->Add(label_23, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	m_IntervalCtrlsSizer->Add(m_IntervalSpinner, 0, wxRIGHT, 5);
	m_IntervalCtrlsSizer->Add(m_IntervalUnitLabel, 1, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_4->Add(m_IntervalCtrlsSizer, 1, wxEXPAND, 0);
	grid_sizer_4->Add(label_20, 0, wxALIGN_CENTER_VERTICAL, 0);
	m_DaysCtrlsSizer->Add(m_SunCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_MonCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_TueCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_WedCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_ThuCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_FriCheck, 0, wxRIGHT, 5);
	m_DaysCtrlsSizer->Add(m_SatCheck, 0, wxRIGHT, 5);
	grid_sizer_4->Add(m_DaysCtrlsSizer, 1, wxEXPAND, 0);
	grid_sizer_4->Add(label_21, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_4->Add(m_RelativeDateCheck, 0, 0, 0);
	grid_sizer_4->Add(label_22, 0, wxALIGN_CENTER_VERTICAL, 0);
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
	if( duration > 0 ) {
		m_duration_hours = duration / 60;
		m_duration_minutes = duration % 60;
	}

	if( m_rec.NotificationTime.Time ) {
		int span = m_rec.StartTime.Time - m_rec.NotificationTime.Time;
		if( span > 0 ) {
			m_reminder_hours = span / 60;
			m_reminder_minutes = span % 60;
		}
		else {
			m_reminder_hours = 0;
			m_reminder_minutes = 15;
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
		case Barry::Calendar::Day:
			m_recur_choice = RC_DAILY;
			m_relative_date = false;
			break;

		case Barry::Calendar::MonthByDate:
			m_recur_choice = RC_MONTHLY;
			m_relative_date = false;
			break;

		case Barry::Calendar::MonthByDay:
			m_recur_choice = RC_MONTHLY;
			m_relative_date = true;
			break;

		case Barry::Calendar::YearByDate:
			m_recur_choice = RC_YEARLY;
			m_relative_date = false;
			break;

		case Barry::Calendar::YearByDay:
			m_recur_choice = RC_YEARLY;
			m_relative_date = true;
			break;

		case Barry::Calendar::Week:
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

	m_strings.SyncToWx();

	return wxDialog::TransferDataToWindow();
}

bool CalendarEditDlg::TransferDataFromWindow()
{
	if( !wxDialog::TransferDataFromWindow() )
		return false;

	return true;
}

