///
/// \file	stdint.h
///		Wrapper header file to aid porting to WinCE
///

/*
    Copyright (C) 2012, RealVNC Ltd.

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

#ifndef STDINT_H
#define STDINT_H

#include <windows.h>

typedef UINT8    uint8_t;
typedef INT8     int8_t;
typedef UINT16   uint16_t;
typedef INT16    int16_t;
typedef UINT32   uint32_t;
typedef INT32    int32_t;
typedef UINT64   uint64_t;
typedef INT64    int64_t;

#endif // STDINT_H