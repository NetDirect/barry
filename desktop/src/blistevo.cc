///
/// \file	blistevo.cc
///		Command line test tool for EvoSources
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "EvoSources.h"
#include <iostream>
#include "i18n.h"

using namespace std;

void DumpItems(const EvoSources::List &list)
{
	EvoSources::List::const_iterator i;
	for( i = list.begin(); i != list.end(); ++i ) {
		cout << "  "
			<< i->m_GroupName << ", "
			<< i->m_SourceName << ", "
			<< i->m_SourcePath << endl;
	}
}

int main()
{
	INIT_I18N();

	EvoSources es;
	cout << "\n" << _C("Addressbook:") << "\n------------\n";
	DumpItems(es.GetAddressBook());
	cout << "\n" << _C("Events:") << "\n-------\n";
	DumpItems(es.GetEvents());
	cout << "\n" << _C("Tasks:") << "\n------\n";
	DumpItems(es.GetTasks());
	cout << "\n" << _C("Memos:") << "\n------\n";
	DumpItems(es.GetMemos());
}

