///
/// \file	getpwuidandroid.cc
///		Replacements for getpwuid*() calls on Android
///

/*
    Copyright (C) 2011, RealVNC Ltd.

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

#include "getpwuid.h"

#include <string.h>
#include <errno.h>

#define SDCARD_PATH "/mnt/sdcard"

static void fillInData(uid_t uid, char* emptyString, char* sdcardPath, struct barry_passwd* pwd)
{
	pwd->pw_name = emptyString;
	pwd->pw_passwd = emptyString;
	pwd->pw_uid = uid;
	pwd->pw_gid = 0;
	pwd->pw_gecos = emptyString;
	// This is the important value for Barry, as it's where to store
	// config files.
	//
	// For Android the best place is probably on the sdcard, as then
	// it doesn't matter which application is calling it, as long
	// as it has android.permission.WRITE_EXTERNAL_STORAGE
	pwd->pw_dir = sdcardPath;
	pwd->pw_shell = emptyString;
}

extern "C" struct barry_passwd *barry_getpwuid(uid_t uid)
{
	static char emptyString[1] = {0};
	static char sdcardString[sizeof(SDCARD_PATH)] = SDCARD_PATH;
	static struct barry_passwd gPasswdStruct;
	fillInData(uid, emptyString, sdcardString, &gPasswdStruct);
	return &gPasswdStruct;
}

extern "C" int barry_getpwuid_r(uid_t uid, struct barry_passwd *pwd,
                   char *buf, size_t buflen, struct barry_passwd **result)
{
	if (buflen < sizeof(SDCARD_PATH))
		return ERANGE;
	memcpy(buf, SDCARD_PATH, sizeof(SDCARD_PATH));
	// Point all the empty entries at the '\0' at the end of the buffer
	fillInData(uid, buf + sizeof(SDCARD_PATH) - 1, buf, pwd);
	*result = pwd;
	return 0;
}
