///
/// \file	ostypes.h
///		Low level types for os wrapper library
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_OSTYPES_H__
#define __BARRYDESKTOP_OSTYPES_H__

#include <string>

// Bitmap field, determining what sync types are supported
// PST = Plugin Sync Type
#define PST_NONE		0x00
#define PST_CONTACTS		0x01
#define PST_EVENTS		0x02
#define PST_NOTES		0x04
#define PST_TODOS		0x08
#define PST_ALL			0x0f
#define PST_DO_NOT_SET		0x10000000

namespace OpenSync { namespace Config {

typedef unsigned int pst_type;

// Handy conversion functions
pst_type PSTString2Type(const std::string &pst_string);
std::string PSTType2String(pst_type types);

}}

#endif

