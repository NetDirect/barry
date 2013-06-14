///
/// \file	i18n.h
///		Common internationalization defines, via gettext
///

/*
    Copyright (C) 2009, Nicolas VIVIEN
    Copyright (C) 2009-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_I18N_H__
#define __BARRY_I18N_H__

#include <config.h>
#include <locale.h>
#include "gettext.h"

#define _(String) gettext (String)
#define N_(String) String

#define INIT_I18N(package) { \
	setlocale(LC_ALL, ""); \
	bindtextdomain(package, LOCALEDIR); \
	bind_textdomain_codeset(package, "UTF-8"); \
	textdomain(package); \
	}

#endif

