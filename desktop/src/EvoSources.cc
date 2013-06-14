///
/// \file	EvoSources.cc
///		A class that creates a list of Evolution data sources.
///

/*
    Copyright (C) 2011-2013, Net Direct Inc. (http://www.netdirect.ca/)
    Based on evolution2_sync.h and list_sources.c from evolution opensync plugin
    by Ian Martin
    Copyright (C) 2009, Ian Martin, licensed under LGPL 2.1

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

#include "../config.h"

#if HAVE_EVOLUTION

//
// Some versions of libical use a ring buffer for the following
// functions, for which the library manages memory.  Somewhere along
// the line, this was fixed so that the application was responsible
// for freeing the strings returned by these functions.  A warning
// was added to the header that would display if HANDLE_LIBICAL_MEMORY
// was not defined.
//
// Even newer versions of evolution-data-server, which this plugin
// depends on, get rid of some of these functions, while newer
// versions of libical add _r variants that implement the
// application-free functinality.
//
// Since this plugin does not use these functions, we disable the
// warning by defining HANDLE_LIBICAL_MEMORY, then include the
// headers.
//

#define HANDLE_LIBICAL_MEMORY 1

#include <glib.h>
#include <libedataserver/eds-version.h>

// For some reason, the evolution-data-server libraries use a floating
// point number in the version numbers, which causes trouble during
// compiles when compared in preprocessor statements.  Define our own
// here, only comparing major and minor, since that's all we should need
// to care about.
#define NON_FLOAT_EDS_CHECK_VERSION(major,minor,micro) \
	(EDS_MAJOR_VERSION > (major) || \
	(EDS_MAJOR_VERSION == (major) && EDS_MINOR_VERSION > (minor)) || \
	(EDS_MAJOR_VERSION == (major) && EDS_MINOR_VERSION == (minor)))

#if NON_FLOAT_EDS_CHECK_VERSION(3,6,0)
#include <libecal/libecal.h>
#include <libebook/libebook.h>
#include <libedataserver/libedataserver.h>

#include "EvoSources.h"

struct EvoFunctions
{
	void (*g_type_init)(void);
};

// helper functions

void FetchSources(EvoFunctions &funcs, EvoSources::List &list,
		ESourceRegistry *reg,
		const gchar *extension_type)
{
	GList *sources = e_source_registry_list_sources(reg, extension_type);

	ESource *source = NULL;
	GList *s = NULL;

	for( s = sources; s; s = s->next ) {
		source = E_SOURCE(s->data);

		EvoSource sitem;
		sitem.m_SourceName = e_source_get_display_name(source);
		sitem.m_SourcePath = e_source_get_uid(source);

		list.push_back(sitem);
	}

	g_list_free_full(sources, g_object_unref);
}

void EvoSources::LoadBaseSyms()
{
	m_funcs->g_type_init = &g_type_init;
}

bool EvoSources::LoadEbookLib()
{
	m_funcs.reset( new EvoFunctions );
	LoadBaseSyms();

	m_funcs->g_type_init();

	GError *gerror = NULL;
	ESourceRegistry *reg = e_source_registry_new_sync(NULL, &gerror);

	FetchSources(*m_funcs, m_addressbook, reg,
		E_SOURCE_EXTENSION_ADDRESS_BOOK);

	g_object_unref(reg);

	return true;
}

bool EvoSources::LoadEcalLib()
{
	m_funcs.reset( new EvoFunctions );
	LoadBaseSyms();

	m_funcs->g_type_init();

	GError *gerror = NULL;
	ESourceRegistry *reg = e_source_registry_new_sync(NULL, &gerror);
	if( !reg ) {
		m_error_msg = gerror->message;
		return false;
	}

	FetchSources(*m_funcs, m_events, reg, E_SOURCE_EXTENSION_CALENDAR);
	FetchSources(*m_funcs, m_tasks, reg, E_SOURCE_EXTENSION_TASK_LIST);
	FetchSources(*m_funcs, m_memos, reg, E_SOURCE_EXTENSION_MEMO_LIST);

	g_object_unref(reg);

	return true;
}



///////////////////////////////////////////////////////////////////////////////
#else	// EDS_CHECK_VERSION(3,6,0)
///////////////////////////////////////////////////////////////////////////////


#include <libecal/e-cal.h>
#include <libebook/e-book.h>
#include <libedataserver/e-data-server-util.h>

#include "EvoSources.h"

struct EvoFunctions
{
	void (*g_type_init)(void);

	gboolean (*e_book_get_addressbooks)(ESourceList**, GError **);

	gboolean (*e_cal_get_sources)(ESourceList **, ECalSourceType, GError **);

	GSList* (*e_source_list_peek_groups)(ESourceList *);
	GSList* (*e_source_group_peek_sources)(ESourceGroup *);
	const gchar* (*e_source_group_peek_name)(ESourceGroup *);
	const gchar* (*e_source_peek_name)(ESource *);
	gchar* (*e_source_get_uri)(ESource *);
};

// helper functions

void FetchSources(EvoFunctions &funcs, EvoSources::List &list,
		ESourceList *sources)
{
	GSList *g = NULL;
	for( g = funcs.e_source_list_peek_groups(sources); g; g = g->next ) {
		ESourceGroup *group = (ESourceGroup*) g->data;

		GSList *s = NULL;
		for( s = funcs.e_source_group_peek_sources(group); s; s = s->next ) {
			EvoSource sitem;
			ESource *source = (ESource*) s->data;

			sitem.m_GroupName = funcs.e_source_group_peek_name(group);
			sitem.m_SourceName = funcs.e_source_peek_name(source);
			sitem.m_SourcePath = funcs.e_source_get_uri(source);

			list.push_back(sitem);
		}
	}
}

void FetchAddressBooks(EvoFunctions &funcs, EvoSources::List &list)
{
	ESourceList *sources = NULL;
	if( funcs.e_book_get_addressbooks(&sources, NULL) ) {
		FetchSources(funcs, list, sources);
	}
}

void FetchCalendars(EvoFunctions &funcs, EvoSources::List &list,
			ECalSourceType source_type)
{
	ESourceList *sources = NULL;
	if( funcs.e_cal_get_sources(&sources, source_type, NULL) ) {
		FetchSources(funcs, list, sources);
	}
}

void EvoSources::LoadBaseSyms()
{
	// Ideally, these functions would be loaded dynamically,
	// but there are two problems with that, which need to be
	// cracked first:
	//
	//     - the glib major version numbers, according to the .so
	//       files, are not consistent between distros
	//     - when used dynamically, in a program that is not otherwise
	//       linked to glib, it can segfault... and it seems to get
	//       funky somewhere inside glib itself... so I'm guessing that
	//       the library is not being initialized properly :-(
	//
	// All this means that general compilations of barrydesktop will
	// need evolution-data-server and friends linked in.  It is possible
	// to compile without, but then you risk getting a weak auto-detection
	// of data sources in newer versions of evolution.  If your data
	// sources are located in paths like this:
	//
	//      file:///home/cdfrey/.evolution/addressbook/local/system
	//
	// then it is safe to disable evolution for the barrydesktop compile.
	// Otherwise, if your version of evolution uses funkier names, or
	// uses local:system, or puts its files under ~/.config, then
	// build with evolution and link against it.
	//
	// Note that if you do not have any OpenSync libraries installed,
	// then you don't need this evolution source detection at all, and
	// you can safely disable evolution for the barrydesktop compile too,
	// since the desktop GUI will not allow the user to even enter the
	// sync screen without OpenSync.
	//

	m_funcs->g_type_init = &g_type_init;
	m_funcs->e_source_list_peek_groups = &e_source_list_peek_groups;
	m_funcs->e_source_group_peek_sources = &e_source_group_peek_sources;
	m_funcs->e_source_group_peek_name = &e_source_group_peek_name;
	m_funcs->e_source_peek_name = &e_source_peek_name;
	m_funcs->e_source_get_uri = &e_source_get_uri;

	m_funcs->e_book_get_addressbooks = &e_book_get_addressbooks;
	m_funcs->e_cal_get_sources = &e_cal_get_sources;
}

bool EvoSources::LoadEbookLib()
{
	m_funcs.reset( new EvoFunctions );
	LoadBaseSyms();

	m_funcs->g_type_init();

	FetchAddressBooks(*m_funcs, m_addressbook);

	return true;
}

bool EvoSources::LoadEcalLib()
{
	m_funcs.reset( new EvoFunctions );
	LoadBaseSyms();

	m_funcs->g_type_init();

	FetchCalendars(*m_funcs, m_events, E_CAL_SOURCE_TYPE_EVENT);
	FetchCalendars(*m_funcs, m_tasks, E_CAL_SOURCE_TYPE_TODO);
	FetchCalendars(*m_funcs, m_memos, E_CAL_SOURCE_TYPE_JOURNAL);

	return true;
}

#endif	// EDS_CHECK_VERSION

void EvoSources::Detect()
{
	Clear();

	if( LoadEbookLib() && LoadEcalLib() ) {
		// done!
		m_supported = true;
	}
	else {
		Clear();
		GuessPaths();
		m_supported = false;
	}
}

bool EvoSources::IsSupported() const
{
	return m_supported;
}




/////////////////////////////////////////////////////////////////////////////
#else // HAVE_EVOLUTION
/////////////////////////////////////////////////////////////////////////////




// No Evolution data libraries available

#include "EvoSources.h"
#include <string>

// helper functions

void EvoSources::Detect()
{
	GuessPaths();
}

bool EvoSources::IsSupported() const
{
	return false;
}



/////////////////////////////////////////////////////////////////////////////
#endif // HAVE_EVOLUTION
/////////////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

using namespace std;

bool EvoSources::PathExists(const std::string &path)
{
	struct stat s;
	if( stat(path.c_str(), &s) == 0 ) {
		if( S_ISDIR(s.st_mode) ) {
			return true;
		}
	}
	return false;
}

void SetIfExists(EvoSources::List &list,
		const std::string &group,
		const std::string &name,
		const std::string &dir)
{
	if( EvoSources::PathExists(dir) ) {
		EvoSource sitem;

		sitem.m_GroupName = group;
		sitem.m_SourceName = name;
		sitem.m_SourcePath = "file://" + dir;

		list.push_back(sitem);
	}
}

EvoSources::EvoSources()
{
	Detect();
}

void EvoSources::GuessPaths()
{
	struct passwd *pw = getpwuid(getuid());
	if( !pw )
		return;

	string base = pw->pw_dir;
	base += "/.evolution/";

	string tail = "/local/system";

	SetIfExists(m_addressbook, "Autodetect", "Addressbook", base + "addressbook" + tail);
	SetIfExists(m_events, "Autodetect", "Events", base + "calendar" + tail);
	SetIfExists(m_tasks, "Autodetect", "Tasks", base + "tasks" + tail);
	SetIfExists(m_memos, "Autodetect", "Memos", base + "memos" + tail);
}

void EvoSources::Clear()
{
	m_addressbook.clear();
	m_events.clear();
	m_tasks.clear();
	m_memos.clear();
}

bool EvoSources::IsEmpty() const
{
	return	m_addressbook.empty() &&
		m_events.empty() &&
		m_tasks.empty() &&
		m_memos.empty();
}

bool EvoSources::IsDefaultable() const
{
	return
		// first three are required
		m_addressbook.size() && m_addressbook[0].m_SourcePath.size() &&
		m_events.size() && m_events[0].m_SourcePath.size() &&
		m_tasks.size() && m_tasks[0].m_SourcePath.size() &&

		// and all lists must not have more than 1 item
		m_addressbook.size() <= 1 &&
		m_events.size() <= 1 &&
		m_tasks.size() <= 1 &&
		m_memos.size() <= 1;
}

