///
/// \file	pppfilter.h
///		Data filter class, to morph PPP data into something that
///		the Blackberry / Rogers / ISP can handle.
///		This logic is based partly on XmBlackBerry's
///		gprs_protocol_fix.c program.
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_M_PPPFILTER_H__
#define __BARRY_M_PPPFILTER_H__

#include "dll.h"
#include "data.h"

namespace Barry {

class BXEXPORT PppFilter
{
private:
	Data m_writeBuf;			// used for 0x7e handling

	// write flags
	bool m_ppp_mode;
	unsigned char m_last;

	BXLOCAL const Data& GetBuffer() const;	// not implemented, since
		// Write can return either m_writeBuf or data, and
		// so this would be useless and unsafe

	BXLOCAL void Filter(Data &dest, const Data &src, unsigned int destoffset);

public:
	PppFilter();

	bool PppMode() const { return m_ppp_mode; }
	const Data& Write(const Data &data);
	Data& Write(const Data &data, unsigned int prepend);
};

} // namespace Barry

#endif

