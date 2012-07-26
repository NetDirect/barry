///
/// \file	r_recur_base-int.h
///		Internal header, with macro for defining FieldHandles in
///		the RecurBase class.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_RECUR_BASE_INT_H__
#define __BARRY_RECORD_RECUR_BASE_INT_H__

#define RECUR_BASE_FIELD_HANDLES \
	FHP(Recurring, _("Recurring Event Flag")); \
	\
	FHE(rbfh_rct, RecurringCodeType, RecurringType, _("Recurrence Type")); \
	FHE_CONST(rbfh_rct, Day, _("Daily")); \
	FHE_CONST(rbfh_rct, MonthByDate, _("Monthly, by Number")); \
	FHE_CONST(rbfh_rct, MonthByDay, _("Monthly, by Weekday")); \
	FHE_CONST(rbfh_rct, YearByDate, _("Yearly, by Date (eg. Every March 5)")); \
	FHE_CONST(rbfh_rct, YearByDay, _("Yearly, by Day (eg. Every 3rd Wed of Jan)")); \
	FHE_CONST(rbfh_rct, Week, _("Weekly")); \
	\
	FHP(Interval, _("Recurring Interval")); \
	FHP(RecurringEndTime, _("Recurring End Time")); \
	FHP(Perpetual, _("Recur Perpetually")); \
	FHP(DayOfWeek, _("Day Of Week (0-6)")); \
	FHP(WeekOfMonth, _("Week Of Month (1-5)")); \
	FHP(DayOfMonth, _("Day Of Month (1-31)")); \
	FHP(MonthOfYear, _("Month Of Year (1-12)")); \
	FHP(WeekDays, _("Weekdays bitmask (bit 0 for Sunday)"));

#endif

