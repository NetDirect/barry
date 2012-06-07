///
/// \file	wrappers.cc
///		Wrapper function implementations to aid porting to WinCE
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

#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <windows.h>
#include <locale.h>
#include <signal.h>
#include <winioctl.h>
// Needs to be last as it re-defines any missing errno
#include <errno.h>

#define MAX_ERR_LEN 128
static char errorString[MAX_ERR_LEN];
#ifdef WINCE
// Various WinCE time functions must be manually implemented, including
// the non-reentrant implementations. These variables are for those.
static tm gLocalTime;

#define MAX_CTIME_LEN 128
static char ctimeBuf[MAX_CTIME_LEN];

#endif

char *strerror(int errnum) {
	sprintf(errorString, "error number %d", errnum);
	return errorString;
}

unsigned int sleep(unsigned int seconds) {
	Sleep(seconds * 1000);
	return 0;
}

int usleep(useconds_t usec) {
	Sleep(usec / 1000);
	return 0;
}

// From http://support.microsoft.com/kb/167296
static void UnixTimeToFileTime(time_t t, LPFILETIME pft) {
	LONGLONG ll;
	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = static_cast<DWORD>(ll);
    pft->dwHighDateTime = static_cast<DWORD>(ll >> 32);
}

static void FileTimeToUnixTime(const LPFILETIME pft, time_t *t) {
	LONGLONG ll;
	ll = (static_cast<LONGLONG>(pft->dwHighDateTime) << 32) |
		static_cast<LONGLONG>(pft->dwLowDateTime);
	// Adjust the offset to 100ns periods from UNIX epoc
	ll -= 116444736000000000;
	// Adjust it to seconds
	ll /= 10000000;
	*t = static_cast<time_t>(ll);
}

static BOOL isLeapYear(DWORD year) {
	if( (year % 400) == 0 )
		return TRUE;
	if( (year % 100) == 0 )
		return FALSE;
	if( (year % 4) == 0 )
		return TRUE;
	return FALSE;
}

/* Static lookup table for how many days in the year each month starts on.
 * Jan = 0, Feb = 1. */
static int monthLookup [] = {
	0,
	31,
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
};

struct tm *localtime_r(const time_t *timep, struct tm *result) {
	FILETIME ft;
	SYSTEMTIME st;
	UnixTimeToFileTime(*timep, &ft);
	FileTimeToSystemTime(&ft, &st);

	result->tm_sec = st.wSecond;
	result->tm_min = st.wMinute;
	result->tm_hour = st.wHour;
	result->tm_mday = st.wDay;
	result->tm_mon = st.wMonth - 1;
	result->tm_year = st.wYear - 1900;
	result->tm_wday = st.wDayOfWeek;
	result->tm_yday = monthLookup[st.wMonth - 1] +
		((isLeapYear(st.wYear) && st.wMonth > 2)? 1 : 0) +
		st.wDay;
	result->tm_isdst = false;

	return result;
}

char *optarg;
int optind = 1;
int opterr = 1;
int optopt = -1;

int getopt(int argc, char * const argv[],
                  const char *optstring)
{
	if( optind >= argc ||
		argv[optind][0] == '-' && argv[optind][1] == '-' ) {
		/* end processing */
		return -1;
	} else if( argv[optind][0] == '-' ) {
		/* Found an option */
		const char optchar = argv[optind][1];
		/* Look for it in optstring */
		while ( *optstring != '\0' ) {
			if( *optstring == optchar )
				break;
			++optstring;
			if( *optstring == ':' )
				++optstring;
		}
		if( *optstring == '\0' ) {
			if( opterr != 0 )
				fprintf(stderr, "Unknown option '%c'\n", optchar);
			optopt = optchar;
			return '?';
		}
		/* See if it has a parameter */
		if( optstring[1] == ':' ) {
			if( optind + 1 >= argc ) {
				if( opterr != 0 )
					fprintf(stderr, "No additional parameter for option '%c'\n", optchar);
				optopt = optchar;
				return '?';
			}
			++optind;
			optarg = argv[optind];
		}
		++optind;
		return optchar;
	} else {
		/* Found a non-option argument, should really move it to the end for complete
		 * compatibility. */
		/* TODO: move it to the end rather than just giving up */
		return -1;
	}

}

#define MAX_LOCALE_LEN 128
static TCHAR localeStrW[MAX_LOCALE_LEN];
static char localeStr[MAX_LOCALE_LEN];

char *setlocale(int category, const char *locale)
{
	// Non-functional on WinCE
	return NULL;
}

sighandler_t signal(int signum, sighandler_t handler)
{
	if( signum == SIGINT ) {
		DWORD bytesReturned = 0;
		if( !DeviceIoControl(
				_fileno(stdout),
				/* This is the value of IOCTL_CONSOLE_SETCONTROLCHANDLER */
				CTL_CODE(FILE_DEVICE_CONSOLE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS),
				(LPVOID)handler, sizeof(handler),
				NULL, 0, &bytesReturned, NULL) ) 
			return SIG_ERR;
		else
			return NULL;
	}
	// All other signal handling is non-functional on WinCE
	return SIG_ERR;
}

#ifdef WINCE
/* WinCE doesn't provide a getenv */
char *getenv(const char *name)
{
	// Pretend there are no environment variables
	return NULL;
}
#endif

#ifdef WINCE
// WinCE declares various time methods but doesn't provide them
// (see http://blogs.msdn.com/b/cenet/archive/2006/04/29/time-h-on-windows-ce.aspx).
struct tm *localtime(const time_t *timep) {
	return localtime_r(timep, &gLocalTime);
}

time_t time(time_t *t) {
	time_t ret = 0;
	
	SYSTEMTIME st;
	FILETIME ft;
	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
	FileTimeToUnixTime(&ft, &ret);

	if (t != NULL)
		*t = ret;
	return ret;
}

// This isn't locale friendly, assuming English weeknames and month names,
// but it seems to be what the standard says.
const static char weekdayStrLookup[7][4] = {
	 "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
const static char monthStrLookup[12][4] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *ctime(const time_t *timep) {
	FILETIME ft;
	SYSTEMTIME st;
	UnixTimeToFileTime(*timep, &ft);
	FileTimeToSystemTime(&ft, &st);
	_snprintf(ctimeBuf, MAX_CTIME_LEN,
		"%s %s %02d %02d:%02d:%02d %04d\n",
		weekdayStrLookup[st.wDayOfWeek], monthStrLookup[st.wMonth - 1],
		st.wDay, st.wHour, st.wMinute, st.wSecond, st.wYear);
	ctimeBuf[MAX_CTIME_LEN - 1] = 0; // Make sure it's NUL terminated
	return ctimeBuf;
}

time_t mktime(struct tm *tm) {
	time_t ret;
	FILETIME ft;
	SYSTEMTIME st;

	// Convert struct tm to a SYSTEMTIME
	st.wDay = tm->tm_mday;
	st.wDayOfWeek = 0;
	st.wHour = tm->tm_hour;
	st.wMilliseconds = 0;
	st.wMinute = tm->tm_min;
	st.wMonth = tm->tm_mon + 1;
	st.wSecond = tm->tm_sec;
	st.wYear = tm->tm_year + 1900;

	// Convert the system time to unix time
	SystemTimeToFileTime(&st, &ft);
	FileTimeToUnixTime(&ft, &ret);
	// Convert the filetime back so we can set tm_wday and tm_yday
	FileTimeToSystemTime(&ft, &st);
	tm->tm_wday = st.wDayOfWeek;
	tm->tm_yday = monthLookup[st.wMonth - 1] +
		((isLeapYear(st.wYear) && st.wMonth > 2)? 1 : 0) +
		st.wDay;

	return ret;
}

// Currently this only implements the strftime format specifiers
// needed by libbarry.
size_t strftime(char *s, size_t max, const char *format,
				const struct tm *tm) {
	char * curOut = s;
	const char* curIn = format;
	while (*curIn != 0 && (size_t)(curOut - s) <= max) {
		if( *curIn == '%' ) {
			const int remainingSpace = max - (curOut - s);
			// Start of format specifier
			switch (curIn[1]) {
				case '%':
					// Escaped %
					*curOut = '%';
					++curOut;
					curIn += 2;
					break;
				case 'Y':
					// Year as decimal, including century
					curOut += _snprintf(curOut, remainingSpace,
						"%04d", tm->tm_year + 1900);
					++curIn;
					break;
				case 'm':
					// Month as a decimal (01-12)
					curOut += _snprintf(curOut, remainingSpace,
						"%02d", tm->tm_mon + 1);
					++curIn;
					break;
				case 'd':
					// Day of month in decimal (01-31)
					curOut += _snprintf(curOut, remainingSpace,
						"%02d", tm->tm_mday);
					++curIn;
					break;
				case 'H':
					// Hour in decimal (00-23)
					curOut += _snprintf(curOut, remainingSpace,
						"%02d", tm->tm_hour);
					++curIn;
					break;
				case 'M':
					// Minutes in decimal (00-59)
					curOut += _snprintf(curOut, remainingSpace,
						"%02d", tm->tm_min);
					++curIn;
					break;
				case 'S':
					// Seconds as a decimal (00-60)
					curOut += _snprintf(curOut, remainingSpace,
						"%02d", tm->tm_sec);
					++curIn;
					break;
				default:
					// Unsupported format specifier, including NUL
					// just write it out as is.
					*curOut = *curIn;
					++curOut;
					++curIn;
					break;
			}
		} else {
			*curOut = *curIn;
			++curOut;
			++curIn;
		}
	}
	// NUL terminate if possible
	if( (size_t)(curOut - s) < max ) {
		*curOut = 0;
		++curOut;
	}
	return (curOut - s);

}

#endif