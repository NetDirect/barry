///
/// \file	libtest.h
///		Routines for testing the Barry libraries
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_LIBTEST_H__
#define __BARRY_LIBTEST_H__

#define TEST(t, err_msg) \
	if( !(t) ) { \
		std::cout << err_msg << std::endl; \
		return false; \
	}

typedef bool (*testfunc)();

void AddTest(const char *name, testfunc test);

struct NewTest
{
	explicit NewTest(const char *name, testfunc test)
	{
		AddTest(name, test);
	}
};

#endif

