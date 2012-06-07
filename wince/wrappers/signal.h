///
/// \file	signal.h
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

#ifndef SIGNAL_H
#define SIGNAL_H

#define SIG_ERR 0

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGTERM 15

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);

#endif // SIGNAL_H