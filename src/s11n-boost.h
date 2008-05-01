///
/// \file	s11n-boost.h
///		Non-intrusive versions of serialization functions for the
///		record classes.  These template functions make it possible
///		to use the record classes with the Boost::Serialization
///		library.
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_S11N_BOOST_H__
#define __BARRY_S11N_BOOST_H__

#include "record.h"
#include <boost/serialization/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
// special versions
//
// BARRY_BASE_S11N_VERSION      - the base version where all record data is
//                                stored so it can be fully retrieved and
//                                uploaded to the handheld device later.
// BARRY_POD_MAP_VERSION        - if these templates are called with a version
//                                equal or greater than this, only mappable,
//                                POD data is included in the serialization
//
#define BARRY_BASE_S11N_VERSION		0
#define BARRY_POD_MAP_VERSION		1000

// namespace boost::serialization, for the non-intrusive version
namespace boost {
namespace serialization {

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::UnknownField &uf, const unsigned int ver)
{
	ar & make_nvp("type", uf.type);
	ar & make_nvp("data", uf.data);
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Contact::GroupLink &g, const unsigned int ver)
{
	ar & make_nvp("Link", g.Link);
	ar & make_nvp("Unknown", g.Unknown);
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Contact &c, const unsigned int ver)
{
	ar & make_nvp("RecType", c.RecType);
	ar & make_nvp("RecordId", c.RecordId);

	ar & make_nvp("Email", c.Email);
	ar & make_nvp("Phone", c.Phone);
	ar & make_nvp("Fax", c.Fax);
	ar & make_nvp("WorkPhone", c.WorkPhone);
	ar & make_nvp("HomePhone", c.HomePhone);
	ar & make_nvp("MobilePhone", c.MobilePhone);
	ar & make_nvp("Pager", c.Pager);
	ar & make_nvp("PIN", c.PIN);
	ar & make_nvp("Radio", c.Radio);
	ar & make_nvp("WorkPhone2", c.WorkPhone2);
	ar & make_nvp("HomePhone2", c.HomePhone2);
	ar & make_nvp("OtherPhone", c.OtherPhone);
	ar & make_nvp("FirstName", c.FirstName);
	ar & make_nvp("LastName", c.LastName);
	ar & make_nvp("Company", c.Company);
	ar & make_nvp("DefaultCommunicationsMethod", c.DefaultCommunicationsMethod);
	ar & make_nvp("Address1", c.WorkAddress.Address1);
	ar & make_nvp("Address2", c.WorkAddress.Address2);
	ar & make_nvp("Address3", c.WorkAddress.Address3);
	ar & make_nvp("City", c.WorkAddress.City);
	ar & make_nvp("Province", c.WorkAddress.Province);
	ar & make_nvp("PostalCode", c.WorkAddress.PostalCode);
	ar & make_nvp("Country", c.WorkAddress.Country);
	ar & make_nvp("JobTitle", c.JobTitle);
	ar & make_nvp("PublicKey", c.PublicKey);
	ar & make_nvp("URL", c.URL);
	ar & make_nvp("Prefix", c.Prefix);
	ar & make_nvp("Categories", c.Categories);
	ar & make_nvp("HomeAddress1", c.HomeAddress.Address1);
	ar & make_nvp("HomeAddress2", c.HomeAddress.Address2);
	ar & make_nvp("HomeAddress3", c.HomeAddress.Address3);
	ar & make_nvp("Notes", c.Notes);
	ar & make_nvp("UserDefined1", c.UserDefined1);
	ar & make_nvp("UserDefined2", c.UserDefined2);
	ar & make_nvp("UserDefined3", c.UserDefined3);
	ar & make_nvp("UserDefined4", c.UserDefined4);
	ar & make_nvp("HomeCity", c.HomeAddress.City);
	ar & make_nvp("HomeProvince", c.HomeAddress.Province);
	ar & make_nvp("HomePostalCode", c.HomeAddress.PostalCode);
	ar & make_nvp("HomeCountry", c.HomeAddress.Country);
	ar & make_nvp("Image", c.Image);
	
	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("GroupLinks", c.GroupLinks);
		ar & make_nvp("Unknowns", c.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::EmailAddress &a, const unsigned int ver)
{
	ar & make_nvp("Name", a.Name);
	ar & make_nvp("Email", a.Email);
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Message &m, const unsigned int ver)
{
	ar & make_nvp("From", m.From);
	ar & make_nvp("To", m.To);
	ar & make_nvp("Cc", m.Cc);
	ar & make_nvp("Sender", m.Sender);
	ar & make_nvp("ReplyTo", m.ReplyTo);
	ar & make_nvp("Subject", m.Subject);
	ar & make_nvp("Body", m.Body);
	ar & make_nvp("Attachment", m.Attachment);
	ar & make_nvp("MessageRecordId", m.MessageRecordId);
	ar & make_nvp("MessageReplyTo", m.MessageReplyTo);
	ar & make_nvp("MessageDateSent", m.MessageDateSent);
	ar & make_nvp("MessageDateReceived", m.MessageDateReceived);

	ar & make_nvp("MessageTruncated", m.MessageTruncated);
	ar & make_nvp("MessageRead", m.MessageRead);
	ar & make_nvp("MessageReply", m.MessageReply);
	ar & make_nvp("MessageSaved", m.MessageSaved);
	ar & make_nvp("MessageSavedDeleted", m.MessageSavedDeleted);

	ar & make_nvp("MessagePriority", m.MessagePriority);
	ar & make_nvp("MessageSensitivity", m.MessageSensitivity);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", m.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Calendar &c, const unsigned int ver)
{
	ar & make_nvp("RecType", c.RecType);
	ar & make_nvp("RecordId", c.RecordId);

	ar & make_nvp("AllDayEvent", c.AllDayEvent);

	ar & make_nvp("Subject", c.Subject);
	ar & make_nvp("Notes", c.Notes);
	ar & make_nvp("Location", c.Location);

	ar & make_nvp("NotificationTime", c.NotificationTime);
	ar & make_nvp("StartTime", c.StartTime);
	ar & make_nvp("EndTime", c.EndTime);
	
	ar & make_nvp("FreeBusyFlag", c.FreeBusyFlag);
	ar & make_nvp("ClassFlag", c.ClassFlag);
	
	ar & make_nvp("Recurring", c.Recurring);
	ar & make_nvp("RecurringType", c.RecurringType);
	ar & make_nvp("Interval", c.Interval);
	ar & make_nvp("RecurringEndTime", c.RecurringEndTime);
	ar & make_nvp("Perpetual", c.Perpetual);
	ar & make_nvp("TimeZoneCode", c.TimeZoneCode);

	ar & make_nvp("DayOfWeek", c.DayOfWeek);
	ar & make_nvp("WeekOfMonth", c.WeekOfMonth);
	ar & make_nvp("DayOfMonth", c.DayOfMonth);
	ar & make_nvp("MonthOfYear", c.MonthOfYear);
	ar & make_nvp("WeekDays", c.WeekDays);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", c.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::ServiceBookConfig &c, const unsigned int ver)
{
	ar & make_nvp("Format", c.Format);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", c.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::ServiceBook &c, const unsigned int ver)
{
	ar & make_nvp("RecType", c.RecType);
	ar & make_nvp("RecordId", c.RecordId);

	ar & make_nvp("Name", c.Name);
	ar & make_nvp("HiddenName", c.HiddenName);
	ar & make_nvp("Description", c.Description);
	ar & make_nvp("DSID", c.DSID);
	ar & make_nvp("BesDomain", c.BesDomain);
	ar & make_nvp("UniqueId", c.UniqueId);
	ar & make_nvp("ContentId", c.ContentId);
	ar & make_nvp("Config", c.Config);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", c.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Memo &m, const unsigned int ver)
{
	ar & make_nvp("RecType", m.RecType);
	ar & make_nvp("RecordId", m.RecordId);

	ar & make_nvp("MemoType", m.MemoType);
	ar & make_nvp("Title", m.Title);
	ar & make_nvp("Body", m.Body);
	ar & make_nvp("Category", m.Category);
	
	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp( "Unknowns", m.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Task &t, const unsigned int ver)
{
	ar & make_nvp("RecType", t.RecType);
	ar & make_nvp("RecordId", t.RecordId);

	ar & make_nvp("TaskType", t.TaskType);
	ar & make_nvp("Summary", t.Summary);
	ar & make_nvp("Notes", t.Notes);
	ar & make_nvp("Categories", t.Categories);
	ar & make_nvp("UID", t.UID);

	ar & make_nvp("StartTime", t.StartTime);
	ar & make_nvp("DueTime", t.DueTime);
	ar & make_nvp("AlarmTime", t.AlarmTime);
	
	ar & make_nvp("TimeZoneCode", t.TimeZoneCode);

	ar & make_nvp("AlarmType", t.AlarmType);
	ar & make_nvp("Interval", t.Interval);
	ar & make_nvp("RecurringType", t.RecurringType);
	ar & make_nvp("RecurringEndTime", t.RecurringEndTime);
	ar & make_nvp("DayOfWeek", t.DayOfWeek);
	ar & make_nvp("WeekOfMonth", t.WeekOfMonth);
	ar & make_nvp("DayOfMonth", t.DayOfMonth);
	ar & make_nvp("MonthOfYear", t.MonthOfYear);
	ar & make_nvp("WeekDays", t.WeekDays);

	ar & make_nvp("ClassType", t.ClassType);
	ar & make_nvp("PriorityFlag", t.PriorityFlag);
	ar & make_nvp("StatusFlag", t.StatusFlag);
	ar & make_nvp("Recurring", t.Recurring);
	ar & make_nvp("Perpetual", t.Perpetual);
	ar & make_nvp("DueDateFlag", t.DueDateFlag);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp( "Unknowns", t.Unknowns);
	}
}

template<class ArchiveT>
void serialize(ArchiveT &ar, Barry::PINMessage &p, const unsigned int ver)
{
	ar & make_nvp("RecType", p.RecType);
	ar & make_nvp("RecordId", p.RecordId);

	ar & make_nvp("From", p.From);
	ar & make_nvp("To", p.To);
	ar & make_nvp("Cc", p.Cc);
	ar & make_nvp("Bcc", p.Bcc);
	ar & make_nvp("Subject", p.Subject);
	ar & make_nvp("Body", p.Body);
	ar & make_nvp("MessageRecordId", p.MessageRecordId);
	ar & make_nvp("MessageReplyTo", p.MessageReplyTo);
	ar & make_nvp("MessageDateSent", p.MessageDateSent);
	ar & make_nvp("MessageDateReceived", p.MessageDateReceived);

	ar & make_nvp("MessageTruncated", p.MessageTruncated);
	ar & make_nvp("MessageRead", p.MessageRead);
	ar & make_nvp("MessageReply", p.MessageReply);
	ar & make_nvp("MessageSaved", p.MessageSaved);
	ar & make_nvp("MessageSavedDeleted", p.MessageSavedDeleted);

	ar & make_nvp("MessagePriority", p.MessagePriority);
	ar & make_nvp("MessageSensitivity", p.MessageSensitivity);

	if(ver < BARRY_POD_MAP_VERSION) {
		ar & make_nvp("Unknowns", p.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::SavedMessage &m, const unsigned int ver)
{
	ar & make_nvp("RecType", m.RecType);
	ar & make_nvp("RecordId", m.RecordId);

	ar & make_nvp("From", m.From);
	ar & make_nvp("To", m.To);
	ar & make_nvp("Cc", m.Cc);
	ar & make_nvp("Bcc", m.Bcc);
	ar & make_nvp("Sender", m.Sender);
	ar & make_nvp("ReplyTo", m.ReplyTo);
	ar & make_nvp("Subject", m.Subject);
	ar & make_nvp("Body", m.Body);
	ar & make_nvp("Attachment", m.Attachment);
	ar & make_nvp("MessageRecordId", m.MessageRecordId);
	ar & make_nvp("MessageReplyTo", m.MessageReplyTo);
	ar & make_nvp("MessageDateSent", m.MessageDateSent);
	ar & make_nvp("MessageDateReceived", m.MessageDateReceived);

	ar & make_nvp("MessageTruncated", m.MessageTruncated);
	ar & make_nvp("MessageRead", m.MessageRead);
	ar & make_nvp("MessageReply", m.MessageReply);
	ar & make_nvp("MessageSaved", m.MessageSaved);
	ar & make_nvp("MessageSavedDeleted", m.MessageSavedDeleted);

	ar & make_nvp("MessagePriority", m.MessagePriority);
	ar & make_nvp("MessageSensitivity", m.MessageSensitivity);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", m.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Folder &f, const unsigned int ver)
{
	ar & make_nvp("RecType", f.RecType);
	ar & make_nvp("RecordId", f.RecordId);

	ar & make_nvp("FolderName", f.FolderName);
	ar & make_nvp("FolderNumber", f.FolderNumber);
	ar & make_nvp("FolderLevel", f.FolderLevel);
	ar & make_nvp("FolderType", f.FolderType);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp( "Unknowns", f.Unknowns);
	}
}

template <class ArchiveT>
void serialize(ArchiveT &ar, Barry::Timezone &t, const unsigned int ver)
{
	ar & make_nvp("RecType", t.RecType);
	ar & make_nvp("RecordId", t.RecordId);

	ar & make_nvp("TZType", t.TZType);
	ar & make_nvp("DSTOffset", t.DSTOffset);
	ar & make_nvp("Index", t.Index);
	ar & make_nvp("Offset", t.Offset);
	ar & make_nvp("OffsetFraction", t.OffsetFraction);
	ar & make_nvp("StartMonth", t.StartMonth);
	ar & make_nvp("EndMonth", t.EndMonth);
	ar & make_nvp("Left", t.Left);
	ar & make_nvp("UseDST", t.UseDST);

	ar & make_nvp("TimeZoneName", t.TimeZoneName);

	if( ver < BARRY_POD_MAP_VERSION ) {
		ar & make_nvp("Unknowns", t.Unknowns);
	}
}

}} // namespace boost::serialization

#endif

