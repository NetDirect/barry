///
/// \file	r_calllog.cc
///		Record parsing class for the phone call logs database.
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "r_calllog.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "iconv.h"
#include <ostream>
#include <iomanip>
#include "ios_state.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

#define MILLISECONDS_IN_A_SECOND 1000

time_t CallLog::GetTime() const
{
	return (time_t)(Timestamp / MILLISECONDS_IN_A_SECOND);
}

CallLog::DirectionFlagType CallLog::DirectionProto2Rec(uint8_t s)
{
	return (DirectionFlagType)s;
}

uint8_t CallLog::DirectionRec2Proto(DirectionFlagType s)
{
	return s;
}

CallLog::PhoneTypeFlagType CallLog::PhoneTypeProto2Rec(uint8_t s)
{
	return (PhoneTypeFlagType)s;
}

uint8_t CallLog::PhoneTypeRec2Proto(PhoneTypeFlagType s)
{
	return s;
}


///////////////////////////////////////////////////////////////////////////////
// CallLog Class

// CallLog Field Codes
#define CLLFC_CALLLOG_TYPE		0x01
#define CLLFC_DIRECTION			0x02
#define CLLFC_DURATION			0x03
#define CLLFC_TIMESTAMP			0x04
#define CLLFC_STATUS			0x06
#define CLLFC_UNIQUEID			0x07
#define CLLFC_PHONE_TYPE		0x0b
#define CLLFC_PHONE_NUMBER		0x0c
#define CLLFC_PHONE_INFO		0x0d
#define CLLFC_CONTACT_NAME		0x1f
#define CLLFC_END			0xffff

static FieldLink<CallLog> CallLogFieldLinks[] = {
    { CLLFC_PHONE_NUMBER,  N_("Phone number"),  0, 0, &CallLog::PhoneNumber, 0, 0, 0, 0, true },
    { CLLFC_CONTACT_NAME,  N_("Contact name"),  0, 0, &CallLog::ContactName, 0, 0, 0, 0, true },
    { CLLFC_END,           N_("End of List"),   0, 0, 0, 0, 0, 0, 0, false }
};

CallLog::CallLog()
{
	Clear();
}

CallLog::~CallLog()
{
}

const unsigned char* CallLog::ParseField(const unsigned char *begin,
				      const unsigned char *end,
				      const IConverter *ic)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )       // if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )   // if field has no size, something's up
		return begin;

	if( field->type == CLLFC_CALLLOG_TYPE ) {
		if( field->u.raw[0] != 'p' ) {
			throw Error( _("CallLog::ParseField: CallLogType is not 'p'") );
		}
		return begin;
	}

	// this is always the same as the RecordID from the lower level
	// protocol, so we throw this away for now
	if( field->type == CLLFC_UNIQUEID)
		return begin;

	// cycle through the type table
	for(    FieldLink<CallLog> *b = CallLogFieldLinks;
		b->type != CLLFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
				return begin;   // done!
			}
			else if( b->timeMember && btohs(field->size) == 4 ) {
				TimeT &t = this->*(b->timeMember);
				t.Time = min2time(field->u.min1900);
				return begin;
			}
		}
	}

	// handle special cases
	switch( field->type )
	{
	case CLLFC_STATUS:
		// single byte... size check above checks for non-zero already
		switch (field->u.raw[0]) {
		case 0x00:
			StatusFlag = Barry::CallLog::OK;
			break;
		case 0x01:
			StatusFlag = Barry::CallLog::Busy;
			break;
		case 0x09:
			StatusFlag = Barry::CallLog::NetError;
			break;
		default:
			StatusFlag = Barry::CallLog::Unknown;
		}
		return begin;

	case CLLFC_DIRECTION:
		if( field->u.raw[0] > CLL_DIRECTION_RANGE_HIGH ) {
			throw Error( _("CallLog::ParseField: direction field out of bounds") );
		}
		else {
			DirectionFlag = DirectionProto2Rec(field->u.raw[0]);
		}
		return begin;

	case CLLFC_PHONE_TYPE:
		if( field->u.raw[0] > CLL_PHONETYPE_RANGE_HIGH ) {
			PhoneTypeFlag = Barry::CallLog::TypeUnknown;
		}
		else {
			PhoneTypeFlag = PhoneTypeProto2Rec(field->u.raw[0]);
		}
		return begin;

	case CLLFC_PHONE_INFO:
		switch (field->u.raw[0]) {
		case 0x03:
			PhoneInfoFlag = Barry::CallLog::InfoKnown;
			break;
		case 0x80:
			PhoneInfoFlag = Barry::CallLog::InfoUnknown;
			break;
		case 0x40:
			PhoneInfoFlag = Barry::CallLog::InfoPrivate;
			break;
		default:
			PhoneInfoFlag = Barry::CallLog::InfoUndefined;
		}
		return begin;

	case CLLFC_DURATION:
		if( btohs(field->size) >= sizeof(field->u.uint32) ) {
			Duration = btohl(field->u.uint32);
			return begin;
		}
		break;

	case CLLFC_TIMESTAMP:
		if( btohs(field->size) >= sizeof(field->u.timestamp) ) {
			Timestamp = btohll(field->u.timestamp);
			return begin;
		}
		break;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void CallLog::ParseHeader(const Data &data, size_t &offset)
{
	// no header in CallLog records
}

void CallLog::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void CallLog::Validate() const
{
}

void CallLog::BuildHeader(Data &data, size_t &offset) const
{
	// not yet implemented
}

void CallLog::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	// not yet implemented
}

void CallLog::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	uint32_t timestamp = Duration;
	int32_t days, hours, minutes, secondes;

	static const char *DirectionName[] = {
		N_("Received"),
		N_("Sent"),
		N_("Call Missing (Messagerie)"),
		N_("Call Missing")
	};
	static const char *StatusName[] = {
		N_("OK"),
		N_("Busy"),
		N_("Error"),
		N_("Not supported by Barry")
	};
	static const char *PhoneInfoName[] = {
		N_("Undefined"),
		N_("Known phone number"),
		N_("Unknown phone number"),
		N_("Private phone number")
	};
	static const char *PhoneTypeName[] = {
		N_("Unknown"),
		N_("Office"),
		N_("Home"),
		N_("Mobile"),
		N_("Not supported by Barry")
	};

	os << _("CallLog entry: ") << "0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

	time_t t = GetTime();
	os << _("   Timestamp: ") << ctime(&t);
	os << _("   Direction: ")
		<< gettext( DirectionName[DirectionFlag] ) << "\n";
	os << _("   Status: ")
		<< gettext( StatusName[StatusFlag] ) << "\n";
	os << _("   Phone info: ")
		<< gettext( PhoneInfoName[PhoneInfoFlag] ) << "\n";
	os << _("   Phone type: ")
		<< gettext( PhoneTypeName[PhoneTypeFlag] ) << "\n";

	os << _("   Duration: ");

	// Days :
	days = (int) (timestamp / (60 * 60 * 24));
	timestamp = timestamp - (days * (60 * 60 * 24));
	// Hours :
	hours = (int) (timestamp / (60 * 60));
	timestamp = timestamp - (hours * (60 * 60));
	// Minutes :
	minutes = (int) (timestamp / 60);
	timestamp = timestamp - (minutes * 60);
	// Secondes :
	secondes = timestamp;

	if (days > 1)
		os << setbase(10) << days << _(" days ");
	else if (days > 0)
		os << setbase(10) << days << _(" day ");

	os << setfill ('0') << setw(2) << setbase(10) << hours;
	os << ":";
	os << setfill ('0') << setw(2) << setbase(10) << minutes;
	os << ":";
	os << setfill ('0') << setw(2) << setbase(10) << secondes;
	os << "\n";

	// cycle through the type table
	for(	const FieldLink<CallLog> *b = CallLogFieldLinks;
		b->type != CLLFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "   " << gettext(b->name) << ": " << s << "\n";
		}
		else if( b->timeMember ) {
			TimeT t = this->*(b->timeMember);
			if( t.Time > 0 )
				os << "   " << gettext(b->name) << ": " << t << "\n";
			else
				os << "   " << gettext(b->name) << ": " << _("unknown") << "\n";
		}
	}


	os << Unknowns;
	os << "\n\n";
}

void CallLog::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	Duration = 0;
	Timestamp = 0;

	ContactName.clear();
	PhoneNumber.clear();

	DirectionFlag = Barry::CallLog::Receiver;
	StatusFlag = Barry::CallLog::Unknown;
	PhoneTypeFlag = Barry::CallLog::TypeUnknown;
	PhoneInfoFlag = Barry::CallLog::InfoUndefined;

	Unknowns.clear();
}

const FieldHandle<CallLog>::ListT& CallLog::GetFieldHandles()
{
	static FieldHandle<CallLog>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME CallLog

	FHP(RecType, _("Record Type Code"));
	FHP(RecordId, _("Unique Record ID"));

	FHD(Duration, _("Duration of Call in Seconds"), CLLFC_DURATION, false);
	FHD(Timestamp, _("Timestamp of Call in Milliseconds"), CLLFC_TIMESTAMP, false);
	FHD(ContactName, _("Contact Name"), CLLFC_CONTACT_NAME, true);
	FHD(PhoneNumber, _("Phone Number"), CLLFC_PHONE_NUMBER, true);

	FHE(dft, DirectionFlagType, DirectionFlag, _("Direction of Call"));
	FHE_CONST(dft, Receiver, _("Received Call"));
	FHE_CONST(dft, Emitter, _("Placed the Call"));
	FHE_CONST(dft, Failed, _("Failed Call"));
	FHE_CONST(dft, Missing, _("Missed Call"));

	FHE(sft, StatusFlagType, StatusFlag, _("Status of Call"));
	FHE_CONST(sft, OK, _("OK"));
	FHE_CONST(sft, Busy, _("Busy"));
	FHE_CONST(sft, NetError, _("Network Error"));
	FHE_CONST(sft, Unknown, _("Unsupported Status"));

	FHE(ptf, PhoneTypeFlagType, PhoneTypeFlag, _("Phone Type"));
	FHE_CONST(ptf, TypeUndefined, _("Undefined"));
	FHE_CONST(ptf, TypeOffice, _("Office"));
	FHE_CONST(ptf, TypeHome, _("Home"));
	FHE_CONST(ptf, TypeMobile, _("Mobile"));
	FHE_CONST(ptf, TypeUnknown, _("Unknown"));

	FHE(pif, PhoneInfoFlagType, PhoneInfoFlag, _("Phone Info"));
	FHE_CONST(pif, InfoUndefined, _("Undefined"));
	FHE_CONST(pif, InfoKnown, _("Phone Number is Set"));
	FHE_CONST(pif, InfoUnknown, _("Phone Number Not Set"));
	FHE_CONST(pif, InfoPrivate, _("Phone Number is Private"));

	FHP(Unknowns, _("Unknown Fields"));

	return fhv;
}

std::string CallLog::GetDescription() const
{
	return ContactName;
}

} // namespace Barry

