///
/// \file	platform.h
///		Platform-specific details
///

/*
    Copyright (C) 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2010, RealVNC Ltd.

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

#ifndef __BARRY_PLATFORM_H__
#define __BARRY_PLATFORM_H__

#include "usbwrap.h"			// USB specific details and windows.h
#include <pthread.h>			// threading and struct timespec

//////////////////////////////////////////////////////////////////////////////
// All GCC specific detail
#if defined( __GNUC__ )

#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define USE_PACK_PRAGMA 0

#else

#define ATTRIBUTE_PACKED
#define USE_PACK_PRAGMA 1

#endif

//////////////////////////////////////////////////////////////////////////////
// All Windows specific detail
#if defined( WIN32 )

// On Windows, we must call usb_set_configuration() before claim_interface()
#define MUST_SET_CONFIGURATION 1

#else

#define MUST_SET_CONFIGURATION 0

#endif



//////////////////////////////////////////////////////////////////////////////
// All FreeBSD / BSD specific detail
#if defined( __FreeBSD__ )

#endif




//////////////////////////////////////////////////////////////////////////////
// All Mac OS X specific detail
#if defined( __APPLE__ ) && defined( __MACH__ )

#endif



#endif

