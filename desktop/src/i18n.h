///
/// \file	i18n.h
///		Internationalization header
///
/// Note: the i18n headers are intentionally split between non-wxWidgets code
///       and wxWidgets code, via i18n.h and wxi18n.h.  This is so that,
///       wxWidgets's version of _() is guaranteed to be removed in favour
///       of our own _W(), and so prevent confusion.  Also, the osyncwrap
///       library code does not strictly depend on wxWidgets, and so should
///       not use any of its headers nor defines nor its wxString.  Keeping
///       libosyncwrap separate should make it easier to turn it into a
///       standalone library someday.
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

#ifndef __BARRY_DESKTOP_I18N_H__
#define __BARRY_DESKTOP_I18N_H__

// Make sure that wxi18n.h is not included in this module
#ifdef __BARRY_DESKTOP_WXI18N_H__
#error Cannot include both i18n.h and wxi18n.h in same module.
#endif

#include <config.h>
#include <locale.h>

// Set the DEFAULT_TEXT_DOMAIN so that gettext.h uses dgettext()
// instead of gettext().  This way we don't have to call textdomain()
// and hope that nobody changes it on us later.
#define DEFAULT_TEXT_DOMAIN PACKAGE
#include "gettext.h"

// Define our own macro for plain const char* strings, so that
// there is no conflict with wxWidgets.  For std::string(_C("blah blah")).
#define _C(x)	gettext(x)

// Convenience macro for main().
#define INIT_I18N(package) { \
	setlocale(LC_ALL, ""); \
	bindtextdomain(package, LOCALEDIR); \
	}

#endif

