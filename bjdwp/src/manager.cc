///
/// \file	manager.cc
///		Application management classes
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

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

#include <iostream>

#include "manager.h"


using namespace std;
using namespace Barry;
using namespace JDG;


namespace JDWP {


// JDWAppInfo class
//------------------


void JDWAppInfo::load(JDGCodInfo &info)
{
	cout << "JDWAppInfo::load" << endl;

	// Assign uniqueId
	uniqueId = info.getUniqueId();

	// Add Class (concat with a previous list)
	JDGClassList *list = &(info.classList);

	classList.insert(classList.end(), list->begin(), list->end());
}


} // namespace JDWP

