///
/// \file	strnlen.c
///		Implementation of strnlen() call, for systems that
///		don't have GNU.
///

/*
    Copyright (C) 2007-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "strnlen.h"

size_t barry_strnlen(const char *s, size_t maxlen)
{
	size_t len = 0;
	while( len < maxlen && s[len] )
		len++;
	return len;
}

