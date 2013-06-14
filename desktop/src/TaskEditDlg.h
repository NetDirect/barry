///
/// \file	TaskEditDlg.h
///		Dialog class to handle the editing of the Task record
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

#ifndef __BARRYDESKTOP_TASK_EDIT_DLG_H__
#define __BARRYDESKTOP_TASK_EDIT_DLG_H__

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


class TaskEditDlg : public wxDialog
{
public:
	// begin wxGlade: TaskEditDlg::ids
	// end wxGlade

private:
	// begin wxGlade: TaskEditDlg::methods
	void set_properties();
	void do_layout();
	// end wxGlade

protected:
	Barry::TimeZones m_static_zones;
	const Barry::TimeZones *m_zones;
	Barry::Task &m_rec;
	StringSync m_strings;
	GUITimeT m_DueDateObj;
	GUITimeT m_ReminderDateObj;
	GUITimeT m_RecurEndDateObj;
	int m_reminder_hours, m_reminder_minutes;
	int m_interval;
	int m_recur_choice;
	bool m_weekdays[7];
	bool m_relative_date;
	std::string m_categories;

	// begin wxGlade: TaskEditDlg::attributes
	wxStaticText* label_1;
	wxTextCtrl* m_TaskSummary;
	wxStaticLine* static_line_1;
	wxStaticText* label_2;
	wxChoice* m_StatusChoice;
	wxStaticText* label_9;
	wxChoice* m_PriorityChoice;
	wxStaticText* label_5;
	wxCheckBox* m_DueCheck;
	wxDatePickerCtrl* m_DueDateCtrl;
	wxSpinCtrl* m_DueHoursSpinner;
	wxStaticText* label_11;
	wxSpinCtrl* m_DueMinutesSpinner;
	wxStaticText* label_8;
	wxChoice* m_TimezoneChoice;
	wxStaticText* label_10;
	wxCheckBox* m_ReminderCheck;
	wxDatePickerCtrl* m_ReminderDateCtrl;
	wxSpinCtrl* m_ReminderHoursSpinner;
	wxStaticText* label_6;
	wxSpinCtrl* m_ReminderMinutesSpinner;
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
	wxStaticText* label_4;
	wxTextCtrl* m_CategoriesText;
	wxStaticText* label_3;
	wxTextCtrl* m_NotesText;
	// end wxGlade

	wxSizer *bottom_buttons;
	wxSizer *m_top_sizer;

	DECLARE_EVENT_TABLE(); // sets to protected:

protected:
	// helper functions
	void RedoLayout();
	void EnableDueDate(bool enable = true);
	void EnableReminderDate(bool enable = true);
	void EnableRecurMode(bool recur = true);

public:
	TaskEditDlg(wxWindow* parent, Barry::Task &rec, bool editable,
		const Barry::TimeZones *device_zones);

	virtual bool TransferDataToWindow();
	virtual bool TransferDataFromWindow();

public:
	virtual void OnDueCheck(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnRecurrenceChoice(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnEndDateCheckbox(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnReminderCheck(wxCommandEvent &event); // wxGlade: <event_handler>
}; // wxGlade: end class


#endif

