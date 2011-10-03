///
/// \file	blistevo.cc
///		Command line test tool for EvoSources
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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
	EvoSources es;
	cout << "\nAddressbook:\n------------\n";
	DumpItems(es.GetAddressBook());
	cout << "\nEvents:\n-------\n";
	DumpItems(es.GetEvents());
	cout << "\nTasks:\n------\n";
	DumpItems(es.GetTasks());
	cout << "\nMemos:\n------\n";
	DumpItems(es.GetMemos());
}

