///
/// \file	time.h
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
#ifndef __SYS_TIME_H__
#define __SYS_TIME_H__

#include <time.h>
#include <stdint.h>
// Should include timespec
#include <pthread.h>
// Should include timeval
#include <winsock2.h>

struct tm *localtime_r(const time_t *timep, struct tm *result);

#endif /* __SYS_TIME_H__ */