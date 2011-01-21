///
/// \file	dll.h
///		Macros for handling DLL/library API visibility
///
/// Based on documentation at: http://gcc.gnu.org/wiki/Visibility
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_DLL_H__
#define __BARRY_DLL_H__

//
//
// Every non-templated class that is meant to be used by an application
// must be declared as:
//
//         class BXEXPORT ClassName {};
//
// Every private (not protected or public) member function of an exported
// class can be declared as:
//
//      private:
//         BXLOCAL void HelperFunc();
//
// Every non-templated function that is meant to be used by an application
// must be declared as:
//
//      BXEXPORT int GetAmount();
//      BXEXPORT std::ostream& operator<< (std::ostream& os, const Obj &obj);
//
//
// Everything else will be hidden, as per the build system's configuration.
//
//

#if __BARRY_HAVE_GCCVISIBILITY__

#define BXEXPORT __attribute__ ((visibility("default")))
#define BXLOCAL __attribute__ ((visibility("hidden")))

#else

#define BXEXPORT
#define BXLOCAL

#endif


//
// Add this to the end of variable argument function declarations.
// For example:
//
//   void log(const char *msg, ...) BARRY_GCC_FORMAT_CHECK(1, 2);
//
// This tells GCC that the first argument is the format string, and
// the second is the first variable argument to check.
//
// If you use this inside a class, you need to allow for the invisible
// 'this' pointer:
//
//   class Trace {
//     public:
//       void logf(const char *msg, ...) BARRY_GCC_FORMAT_CHECK(2, 3);
//   };
//
#if __GNUC__
#define BARRY_GCC_FORMAT_CHECK(a,b) __attribute__ ((format(printf, a, b)))
#else
#define BARRY_GCC_FORMAT_CHECK(a,b)
#endif

#endif

