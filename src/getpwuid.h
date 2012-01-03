///
/// \file	getpwuid.h
///		Header for getpwduid*() calls, for systems that don't have an equivalent.
///

/*
    Copyright (C) 2007-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Portions Copyright (C) 2011, RealVNC Ltd.

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

#ifndef __BARRY_GETPWUID_H__
#define __BARRY_GETPWUID_H__

#include "config.h"		// getpwuid.h is not installed, so this is safe
#include <sys/types.h>

#ifdef HAVE_GETPWUID
// System supports pwd so just include the system headers
#include <pwd.h>

#else

// so define our own version...
#ifdef __cplusplus
extern "C" {
#endif
struct barry_passwd {
	char   *pw_name;       /* username */
	char   *pw_passwd;     /* user password */
	uid_t   pw_uid;        /* user ID */
	gid_t   pw_gid;        /* group ID */
	char   *pw_gecos;      /* real name */
	char   *pw_dir;        /* home directory */
	char   *pw_shell;      /* shell program */
};

struct barry_passwd *barry_getpwuid(uid_t uid);
int barry_getpwuid_r(uid_t uid, struct barry_passwd *pwd,
                   char *buf, size_t buflen, struct barry_passwd **result);

#ifdef __cplusplus
}
#endif

// and override the system's names so we call our own
#define passwd barry_passwd
#define getpwuid(u) barry_getpwuid(u)
#define getpwuid_r(u, p, b, l, r) barry_getpwuid_r(u, p, b, l, r)

#endif // !HAVE_GETPWDUID

#endif	// __BARRY_GETPWUID_H__

