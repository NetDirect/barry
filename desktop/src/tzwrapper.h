///
/// \file	tzwrapper.h
///		Timezone adjustment class, wrapping the TZ environment
///		variable to make struct tm -> time_t conversions easier.
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>, To God be the glory
    Released to the public domain.
*/

#ifndef __TZWRAPPER_H__

#include <string>
#include <time.h>
#include <stdlib.h>

namespace reuse {

/// Parses ISO timestamp in the format of YYYYMMDDTHHMMSS[Z]
/// and places broken down time in result.
/// The trailing Z is optional in the format.
/// If the Z exists, utc will be set to true, otherwise false.
/// Returns NULL on error.
/// Thread-safe.
struct tm* iso_to_tm(const char *timestamp,
			struct tm *result,
			bool &utc);

/// utc_mktime() converts a struct tm that contains
/// broken down time in utc to a time_t.  This function uses
/// a brute-force method of conversion that does not require
/// the environment variable TZ to be changed at all, and is
/// therefore slightly more thread-safe in that regard.
///
/// The difference between mktime() and utc_mktime() is that
/// standard mktime() expects the struct tm to be in localtime,
/// according to the current TZ and system setting, while utc_mktime()
/// always assumes that the struct tm is in UTC, and converts it
/// to time_t regardless of what TZ is currently set.
///
/// The difference between utc_mktime() and TzWrapper::iso_mktime()
/// is that iso_mktime() will parse straight from an ISO string,
/// and if the ISO timestamp ends in a 'Z', it will behave like
/// utc_mktime() except it will alter the TZ environment variable
/// to do it.  If the ISO timestamp has no 'Z', then iso_mktime()
/// behaves like mktime().
///
time_t utc_mktime(struct tm *utctime);

//
// class TzWrapper
//
/// Wrapper class for the TZ environment variable.  This class allows
/// setting TZ to any number of variables, and will restore the original
/// setting on destruction.
///
/// By default, TzWrapper does not change the environment at all, but
/// only saves it.  Alternately, you can use the timezone constructor
/// to save and set a new timezone on the fly.
///
/// Each Set() and Unset() function returns a reference to TzWrapper,
/// so that you can chain function calls like this:
///
///	time_t utc = TzWrapper("Canada/Pacific").mktime(&pacific_tm);
///
/// In addition, there are two static utility functions used to
/// convert ISO timestamps to struct tm* and time_t values.
///
/// Note: This class is not thread-safe, since it modifies the TZ
///       environment variable without locking.  If other threads
///       use time functions, this may interfere with their behaviour.
///
class TzWrapper
{
	std::string m_orig_tz;
	bool m_tz_exists;
	bool m_dirty;

protected:
	void SaveTz()
	{
		char *ptz = getenv("TZ");
		if( ptz )
			m_orig_tz = ptz;
		m_tz_exists = ptz;
	}

	void RestoreTz()
	{
		if( m_dirty ) {
			if( m_tz_exists )
				Set(m_orig_tz.c_str());
			else
				Unset();

			m_dirty = false;
		}
	}

public:
	/// Does not change TZ, only saves current setting
	TzWrapper()
		: m_dirty(false)
	{
		SaveTz();
	}

	/// Saves current setting and sets TZ to new timezone value.
	/// If timezone is null, it is the same as calling Unset().
	explicit TzWrapper(const char *timezone)
		: m_dirty(false)
	{
		SaveTz();
		Set(timezone);
	}

	~TzWrapper()
	{
		RestoreTz();
	}

	/// Set TZ to a new value.  If timezone is null, it is the
	/// same as calling Unset().
	///
	/// If timezone is an empty or invalid timezone string, it
	/// is the same as calling SetUTC().
	TzWrapper& Set(const char *timezone)
	{
		if( timezone )
			setenv("TZ", timezone, 1);
		else
			unsetenv("TZ");
		tzset();
		m_dirty = true;
		return *this;
	}

	/// Deletes TZ from the environment, which has the same effect
	/// as calling SetSysLocal().  This is not a permanent
	/// condition, since TZ will be restored to original state
	/// upon destruction.
	TzWrapper& Unset()
	{
		unsetenv("TZ");
		tzset();
		m_dirty = true;
		return *this;
	}

	/// Set timezone to UTC
	TzWrapper& SetUTC()
	{
		return Set("");
	}

	/// Use system localtime.  This overrides any TZ value that the
	/// user may have set before running your program.
	TzWrapper& SetSysLocal()
	{
		return Unset();
	}

	/// Use the default TZ value that the user set before running
	/// this program.  In most cases, this will be the user's
	/// preferred local timezone.
	TzWrapper& SetDefault()
	{
		RestoreTz();
		return *this;
	}
	/// Same as SetDefault()
	TzWrapper& SetOrig()
	{
		return SetDefault();
	}

	//
	// C library wrappers, for calls like:
	//	time_t t = TzWrapper("Canada/Pacific").mktime(tm);
	//
	char* asctime(const struct tm *t) const { return ::asctime(t); }
	char* asctime_r(const struct tm *t, char *buf) const
		{ return ::asctime_r(t, buf); }
	char* ctime(const time_t *t) const { return ::ctime(t); }
	char* ctime_r(const time_t *t, char *buf) const
		{ return ::ctime_r(t, buf); }
	struct tm* gmtime(const time_t *t) const { return ::gmtime(t); }
	struct tm* gmtime_r(const time_t *t, struct tm *result) const
		{ return ::gmtime_r(t, result); }
	struct tm* localtime(const time_t *t) const { return ::localtime(t); }
	struct tm* localtime_r(const time_t *t, struct tm *result) const
		{ return ::localtime_r(t, result); }
	time_t mktime(struct tm *t) { return ::mktime(t); }

	//
	// Additional utility functions
	//

	/// Converts an ISO timestamp (YYYYMMDDTHHMMWW[Z]) into a
	/// unix time_t.  If the 'Z' UTC flag is not specified, then
	/// the timestamp will be assumed to be in the current
	/// default timezone.  Otherwise, SetUTC() will be used for the
	/// conversion.
	///
	/// This function uses an internal TzWrapper to adjust TZ
	/// if necessary, which is why it is a static function
	/// of TzWrapper, instead of a standalone function.
	///
	static time_t iso_mktime(const char *timestamp);
};

} // namespace reuse

#endif

