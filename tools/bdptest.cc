/**
 * @file bdptest.cc
 * @author Nicolas VIVIEN
 * @date 2009-08-01
 *
 * @note CopyRight Nicolas VIVIEN
 *
 * @brief COD debug file parser
 *   RIM's JDE generates several files when you build a COD application.
 *   Indeed, with the COD files for the device, we have a ".debug" file.
 *   This file is usefull to debug an application from JVM.
 *   This tool is a parser to understand these ".debug" files.
 *
 *   This tool is simply a test application.
 *
 * @par Modifications
 *   - 2009/08/01 : N. VIVIEN
 *     - First release
 *
 * @par Licences
 *   Copyright (C) 2009-2010, Nicolas VIVIEN
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *   See the GNU General Public License in the COPYING file at the
 *   root directory of this project for more details.
 */



#include <iostream>
#include <fstream>
#include <string>
#include "i18n.h"

#include <barry/barry.h>


using namespace std;
using namespace Barry;


int main(int argc, char *argv[], char *envp[])
{
	INIT_I18N(PACKAGE);

	Barry::Init(true, &cout);

	JDG::DebugFileList list;

	JDG::SearchDebugFile(list);

	cout << _("List of debug files: ") << endl;
	cout << list << endl;


	JDG::CodInfo info;

	JDG::LoadDebugInfo(list, argv[1], info);

	return 0;
}

