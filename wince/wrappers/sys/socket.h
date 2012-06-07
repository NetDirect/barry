///
/// \file	socket.h
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

#ifndef __SYS_SOCKET_H__
#define __SYS_SOCKET_H__

#include <winsock2.h>

#define SHUT_RD    0
#define SHUT_WR    1
#define SHUT_RDWR  2

#endif /* __SYS_SOCKET_H__ */