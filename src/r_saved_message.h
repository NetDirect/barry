///
/// \file	r_saved_message.h
///		Blackberry database record parser class for saved email
///		message records.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2007, Brian Edginton (edge@edginton.net)

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

#ifndef __BARRY_RECORD_SAVED_MESSAGE_H__
#define __BARRY_RECORD_SAVED_MESSAGE_H__

#include "dll.h"
#include "r_message_base.h"

namespace Barry {

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//

/// \addtogroup RecordParserClasses
/// @{

class BXEXPORT SavedMessage : public MessageBase
{
private:
	static std::vector<FieldHandle<SavedMessage> > m_FieldHandles;

protected:
	static void FillHandles();

public:
	SavedMessage()
	{
		Clear();
	}

	void Clear()
	{
		MessageBase::Clear();

		RecType = GetDefaultRecType();
		RecordId = 0;
	}

	// database name
	static const char * GetDBName() { return "Saved Email Messages"; }
	static uint8_t GetDefaultRecType() { return 3; }

	// Generic Field Handle support
	static const std::vector<FieldHandle<SavedMessage> >& GetFieldHandles()
	{
		if( !m_FieldHandles.size() )
			FillHandles();
		return m_FieldHandles;
	}
	static void ClearFieldHandles() { m_FieldHandles.clear(); }
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const SavedMessage &msg) {
	msg.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif // __BARRY_RECORD_SAVED_MESSAGE_H__


