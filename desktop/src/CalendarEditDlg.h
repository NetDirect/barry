///
/// \file	CalendarEditDlg.h
///		Dialog class to handle the editing of the Calendar record
///

/*
    Copyright (C) 2012-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_CALENDAR_EDIT_DLG_H__
#define __BARRYDESKTOP_CALENDAR_EDIT_DLG_H__

//#define wxUSE_DATEPICKCTRL 1

#include "StringSync.h"
#include "guitimet.h"
#include <wx/wx.h>
#include <wx/image.h>
#include <barry/barry.h>
// begin wxGlade: ::dependencies
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/datectrl.h>
// end wxGlade
#include <wx/dateevt.h>



// begin wxGlade: ::extracode
// end wxGlade


class CalendarEditDlg: public wxDialog
{
public:
	// begin wxGlade: CalendarEditDlg::ids
	// end wxGlade

private:
	// begin wxGlade: CalendarEditDlg::methods
	void set_properties();
	void do_layout();
	// end wxGlade

protected:
	Barry::TimeZones m_static_zones;
	const Barry::TimeZones *m_zones;
	Barry::Calendar &m_rec;
	StringSync m_strings;
	GUITimeT m_StartDateObj;
	GUITimeT m_EndDateObj;
	GUITimeT m_RecurEndDateObj;
	int m_duration_hours, m_duration_minutes;
	int m_reminder_hours, m_reminder_minutes;
	int m_interval;
	int m_recur_choice;
	bool m_weekdays[7];
	bool m_relative_date;
	std::string m_organizer, m_invited, m_accepted_by;

	// begin wxGlade: CalendarEditDlg::attributes
	wxStaticText* label_1;
	wxTextCtrl* m_Subject;
	wxStaticText* label_2;
	wxTextCtrl* m_Location;
	wxStaticLine* static_line_1;
	wxStaticText* label_4;
	wxCheckBox* m_AllDayCheck;
	wxStaticText* label_5;
	wxDatePickerCtrl* m_StartDateCtrl;
	wxSpinCtrl* m_StartHoursSpinner;
	wxStaticText* label_11;
	wxSpinCtrl* m_StartMinutesSpinner;
	wxStaticText* label_6;
	wxDatePickerCtrl* m_EndDateCtrl;
	wxSpinCtrl* m_EndHoursSpinner;
	wxStaticText* label_12;
	wxSpinCtrl* m_EndMinutesSpinner;
	wxStaticText* label_7;
	wxSpinCtrl* m_DurationHoursSpinner;
	wxStaticText* label_13;
	wxSpinCtrl* m_DurationMinutesSpinner;
	wxStaticText* label_17;
	wxStaticText* label_8;
	wxChoice* m_TimezoneChoice;
	wxStaticText* label_9;
	wxChoice* m_ShowAsChoice;
	wxStaticText* label_10;
	wxSpinCtrl* m_ReminderHoursSpinner;
	wxStaticText* label_13_copy;
	wxSpinCtrl* m_ReminderMinutesSpinner;
	wxStaticText* label_17_copy;
	wxStaticLine* static_line_2;
	wxStaticText* label_18;
	wxChoice* m_RecurrenceChoice;
	wxStaticText* RecurIntervalLabel;
	wxStaticText* RecurIntervalLabelB;
	wxSpinCtrl* m_IntervalSpinner;
	wxStaticText* m_IntervalUnitLabel;
	wxStaticText* RecurDaysLabel;
	wxCheckBox* m_SunCheck;
	wxCheckBox* m_MonCheck;
	wxCheckBox* m_TueCheck;
	wxCheckBox* m_WedCheck;
	wxCheckBox* m_ThuCheck;
	wxCheckBox* m_FriCheck;
	wxCheckBox* m_SatCheck;
	wxStaticText* RecurRelativeDateLabel;
	wxCheckBox* m_RelativeDateCheck;
	wxStaticText* RecurEndDateLabel;
	wxCheckBox* m_NeverEndsCheck;
	wxDatePickerCtrl* m_RecurEndDateCtrl;
	wxStaticLine* static_line_3;
	wxStaticText* label_14;
	wxTextCtrl* m_OrganizerText;
	wxStaticText* label_15;
	wxTextCtrl* m_InvitedText;
	wxStaticText* label_16;
	wxTextCtrl* m_AcceptedByText;
	wxStaticLine* static_line_4;
	wxRadioBox* m_ClassRadioBox;
	wxStaticLine* static_line_5;
	wxStaticText* label_3;
	wxTextCtrl* m_NotesText;
	// end wxGlade

	wxSizer *bottom_buttons;
	wxSizer *m_top_sizer;

	DECLARE_EVENT_TABLE(); // sets to protected:

protected:
	// helper functions
	void UpdateDuration();
	void UpdateEndDate();
	void RedoLayout();
	void EnableAllDayMode(bool all_day = true);
	void EnableRecurMode(bool recur = true);

public:
	CalendarEditDlg(wxWindow* parent, Barry::Calendar &rec, bool editable,
		const Barry::TimeZones *device_zones);

	virtual bool TransferDataToWindow();
	virtual bool TransferDataFromWindow();

public:
	virtual void OnAllDayEvent(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnStartDateChanged(wxDateEvent &event); // wxGlade: <event_handler>
	virtual void OnStartHoursSpin(wxSpinEvent &event); // wxGlade: <event_handler>
	virtual void OnStartMinutesSpin(wxSpinEvent &event); // wxGlade: <event_handler>
	virtual void OnEndDateChanged(wxDateEvent &event); // wxGlade: <event_handler>
	virtual void OnEndHoursSpin(wxSpinEvent &event); // wxGlade: <event_handler>
	virtual void OnEndMinutesSpin(wxSpinEvent &event); // wxGlade: <event_handler>
	virtual void OnDurationHoursSpin(wxSpinEvent &event); // wxGlade: <event_handler>
	virtual void OnDurationMinutesSpin(wxSpinEvent &event); // wxGlade: <event_handler>
	virtual void OnRecurrenceChoice(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnEndDateCheckbox(wxCommandEvent &event); // wxGlade: <event_handler>
}; // wxGlade: end class


#endif

