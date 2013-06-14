///
/// \file	blistevo.cc
///		Command line test tool for EvoSources
///

/*
    Copyright (C) 2011-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
	setlocale(LC_ALL, "");
	INIT_I18N();

	const char *separator = "\n-----------------------------------------\n";

	EvoSources es;
	cout << _C("Defaultable: ") << (es.IsDefaultable() ?
					_C("Yes") : _C("No")) << endl;
	cout << _C("Error during detect: ")
		<< (es.GetErrorMsg().size() ?
			es.GetErrorMsg().c_str() : _C("(none)"))
		<< endl;
	cout << _C("Results:") << "\n";

	cout << "\n" << _C("Addressbook:") << separator;
	DumpItems(es.GetAddressBook());
	cout << "\n" << _C("Events:") << separator;
	DumpItems(es.GetEvents());
	cout << "\n" << _C("Tasks:") << separator;
	DumpItems(es.GetTasks());
	cout << "\n" << _C("Memos:") << separator;
	DumpItems(es.GetMemos());
}

