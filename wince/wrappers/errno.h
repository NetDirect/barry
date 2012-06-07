///
/// \file	errno.h
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

#ifndef __ERRNO_H__
#define __ERRNO_H__

// Define some standard error numbers which might not be defined on all platforms
#ifndef EIO
#define EIO 5
#endif 
#ifndef EINVAL
#define EINVAL 22
#endif 
#ifndef EACCES
#define EACCES 13
#endif 
#ifndef ENODEV
#define ENODEV 19
#endif
#ifndef EBUSY
#define EBUSY 16
#endif 
#ifndef EPIPE
#define EPIPE 32
#endif 
#ifndef EINTR
#define EINTR 4
#endif 
#ifndef ENOMEM
#define ENOMEM 12
#endif 
#ifndef ENOSYS
#define ENOSYS 38
#endif 
#ifndef ENOENT
#define ENOENT 2
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 110 
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 75
#endif 

char *strerror(int errnum);

#endif /* __ERRNO_H__ */