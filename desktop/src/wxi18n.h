///
/// \file	wxi18n.h
///		Internationalization header for wxWidgets code
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

#ifndef __BARRY_DESKTOP_WXI18N_H__
#define __BARRY_DESKTOP_WXI18N_H__

// Make sure that i18n.h is not included in this module
#ifdef __BARRY_DESKTOP_I18N_H__
#error Cannot include both i18n.h and wxi18n.h in same module.
#endif

#include <config.h>
#include <locale.h>

// For the osyncwrap library, it uses gettext, not wxWidgets, so we need
// to make sure we initialize gettext if it is available.
//
// The bindtextdomain define was taken from gettext.h.
#if ENABLE_NLS
#include <libintl.h>
#else
# define bindtextdomain(Domainname, Dirname) \
    ((void) (Domainname), (const char *) (Dirname))
#endif

// Make sure wxWidgets' version of _() does not affect us
#include <wx/wx.h>
#undef _

// Define our own _W() macro, used wherever we'd normally use _T() style
// strings, which creates a wxString on the fly.
// For wxString(_W("blah blah"))
#define _W(x)	wxString(wxGetTranslation(_T(x)))

// For plain old const char* strings...
// Note! this returns a temporary const string!
// Do not rely on its existence beyond the statement it lives in.
#define _C(x)	wxString(wxGetTranslation(_T(x))).utf8_str().data()

// Convenience macro to initialize i18n, using wxWidgets
#define INIT_I18N(package) { \
	/* Do gettext first, if possible, for libosyncwrap */ \
	setlocale(LC_ALL, ""); \
	bindtextdomain("barryosyncwrap", LOCALEDIR); \
	/* Init wxWidgets */ \
	static wxLocale locale; \
	locale.Init(); \
	locale.AddCatalogLookupPathPrefix(_T(LOCALEDIR)); \
	locale.AddCatalog(_T(package)); \
	}

#endif

