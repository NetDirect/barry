///
/// \file	unistd.h
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

#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <windows.h>

typedef DWORD off_t;
typedef DWORD useconds_t;

unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

int getopt(int argc, char * const argv[],
                  const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

/* Technically part of stdlib.h, but here is as good a place as any */
char *getenv(const char *name);

#define snprintf _snprintf

#endif /* __UNISTD_H__ */