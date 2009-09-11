/** 
 * @file dp_codinfo.cc
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
#include <sstream>
#include <fstream>
#include <iomanip>

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "dp_parser.h"
#include "dp_codinfo.h"


using namespace std;


namespace Barry {

namespace JDG {


// Public API
//------------

#define DEBUG_FILE_EXT		".debug"


void SearchDebugFile(DebugFileList &list)
{
	DIR *path;
	struct dirent *entry;

	path = opendir(".");

	while( (entry = readdir(path)) ) {
		int offset;

		if (strlen(entry->d_name) < strlen(DEBUG_FILE_EXT))
			continue;

		offset = strlen(entry->d_name) - strlen(DEBUG_FILE_EXT);

		if (!strcmp(entry->d_name + offset, DEBUG_FILE_EXT)) {
			ifstream file(entry->d_name);

			CodInfo info;

			// Parse header section
			info.ParseHeaderSection(file);

			// Add element to list
			list.AddElement(info.GetUniqueId(), info.GetAppName(), entry->d_name);
		}
	}

	closedir(path);
}


bool LoadDebugInfo(DebugFileList &list, const char *filename, CodInfo &info)
{
	if (filename == NULL)
		return false;

	vector<DebugFileEntry>::iterator b = list.begin();

	for( ; b != list.end(); b++ ) {
		DebugFileEntry entry = (*b);

		if (entry.fileName == string(filename)) {
			info.LoadDebugFile(filename);
			return true;
		}
	}

	return false;
}


bool LoadDebugInfo(DebugFileList &list, const uint32_t uniqueId, const std::string module, CodInfo &info)
{
	vector<DebugFileEntry>::iterator b = list.begin();

	for( ; b != list.end(); b++ ) {
		DebugFileEntry entry = (*b);

		if ((entry.uniqueId == uniqueId) && (entry.appName == module)) {
			info.LoadDebugFile(entry.fileName.c_str());
			return true;
		}
	}

	return false;
}


// DebugFileList class
//------------------------

void DebugFileList::AddElement(uint32_t uniqueid, std::string appname, std::string filename)
{
	DebugFileEntry entry;

	entry.uniqueId = uniqueid;
	entry.appName = appname;
	entry.fileName = filename;

	push_back(entry);
}


void DebugFileList::Dump(std::ostream &os) const
{
	const_iterator i = begin(), e = end();

	os << "  UniqueID  " << "|";
	os << "        Module Name       " << "|";
	os << "         File Name        " << endl;

	os << "------------+";
	os << "--------------------------+";
	os << "--------------------------";
	os << endl;

	for( ; i != e; ++i ) {
		(*i).Dump(os);
	}
}


void DebugFileEntry::Dump(std::ostream &os) const
{
	os << " 0x" << setfill('0') << setw(8) << hex << uniqueId << " |";
	os << " " << appName << setfill(' ') << setw(24) << " |";
	os << " " << fileName << endl;
}


// ClassList class
//---------------------------


void ClassList::CreateDefaultEntries()
{
	ClassEntry entry;

	// 1
	entry.classPath = "com.rim.resources";
	entry.className = "net_rim_rimsecuridlibRIMResources";
	push_back(entry);

	// 2
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimSecurIDLib";
	push_back(entry);

	// 3
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimDatabaseFullException";
	push_back(entry);

	// 4
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimDecryptFailException";
	push_back(entry);

	// 5
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimDuplicateNameException";
	push_back(entry);

	// 6
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimDuplicateTokenException";
	push_back(entry);

	// 7
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimInvalidParamException";
	push_back(entry);

	// 8
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimSecurIDLib";
	push_back(entry);

	// 9
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimWrongDeviceIDException";
	push_back(entry);

	// 10
	entry.classPath = "net.rim.device.cldc.impl.softtoken.rimsecuridlib";
	entry.className = "RimWrongFormFactorException";
	push_back(entry);
}


// CodInfo class
//------------------------

bool CodInfo::LoadDebugFile(const char *filename)
{
	if (filename == NULL)
		return false;

	ifstream file(filename);

	// Parse header file
	ParseHeaderSection(file);

	// Parse type area zone
	ParseTypeSection(file);

	return true;
}


uint32_t CodInfo::GetUniqueId() 
{
	return uniqueId;
}


string CodInfo::GetAppName() 
{
	return appName;
}


// Private API - Section parsing
//-------------------------------

void CodInfo::ParseHeaderSection(ifstream &input) 
{
	uint32_t type;

	type = ParseNextHeaderField(input);

	if (type != COD_DEBUG_UNIQUEID_HEADERFIELD)
		return;

	type = ParseNextHeaderField(input);

	if (type != COD_DEBUG_APPNAME_HEADERFIELD)
		return;
}


void CodInfo::ParseTypeSection(ifstream &input) 
{
	uint32_t type;
	uint32_t count;
	uint32_t nbr, check;

	// Read number of declared type content into this section
	nbr = ParseInteger(input);

	// Read each object
	count = 0;

	while (!input.eof()) {
		type = ParseNextTypeField(input);

		if (type == COD_DEBUG_NONE_FIELD)
			break;

		count++;
	}

	// Read again number of declared type content into this section
	// We have to find the same value
	check = ParseInteger(input);

	// Checking...
	cout << "Nbr = " << dec << nbr << " / Count = " << dec << count << " / check = " << check << endl;
}


uint32_t CodInfo::ParseNextHeaderField(ifstream &input) 
{
	uint32_t type = ParseInteger(input);

	switch (type) {
	case COD_DEBUG_UNIQUEID_HEADERFIELD:
		ParseUniqueId(input);
		break;

	case COD_DEBUG_APPNAME_HEADERFIELD:
		ParseAppName(input);
		break;

	default:
		type = 0xFFFFFFFF;
	}

	return type;
}


uint32_t CodInfo::ParseNextTypeField(ifstream &input) 
{
	uint32_t type = ParseInteger(input);

	switch (type) {
	case COD_DEBUG_NONE_FIELD:
		break;

	case COD_DEBUG_BOOLEAN_FIELD:
		ParseBoolean(input);
		break;

	case COD_DEBUG_BYTE_FIELD:
		ParseByte(input);
		break;

	case COD_DEBUG_CHAR_FIELD:
		ParseChar(input);
		break;

	case COD_DEBUG_SHORT_FIELD:
		ParseShort(input);
		break;

	case COD_DEBUG_INT_FIELD:
		ParseInt(input);
		break;

	case COD_DEBUG_LONG_FIELD:
		ParseLong(input);
		break;

	case COD_DEBUG_CLASS_FIELD:
		ParseClass(input);
		break;

	case COD_DEBUG_ARRAY_FIELD:
		ParseArray(input);
		break;

	case COD_DEBUG_VOID_FIELD:
		ParseVoid(input);
		break;

	case COD_DEBUG_DOUBLE_FIELD:
		ParseDouble(input);
		break;

	default:
		cout << "Type unknown ! " << hex << type << endl;
		type = 0xFFFFFFFF;
	}

	return type;
}


void CodInfo::ParseUniqueId(ifstream &input) 
{
	uniqueId = ParseInteger(input);
}


void CodInfo::ParseAppName(ifstream &input) 
{
	uint32_t len = ParseInteger(input);

	appName = ParseString(input, len);
}


void CodInfo::ParseBoolean(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseBoolean" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseByte(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseByte" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseChar(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseChar" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseShort(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseShort" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseInt(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseInt" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseLong(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseLong" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseClass(ifstream &input) 
{
	uint32_t len;

	ClassEntry object;

	cout << "JDG::CodInfo::ParseClass" << endl;

	len  = ParseInteger(input);

	object.className = ParseString(input, len);

	object.type = ParseInteger(input);
	object.unknown02 = ParseInteger(input);
	object.unknown03 = ParseInteger(input);
	object.id = ParseInteger(input);
	
	len  = ParseInteger(input);

	if (len == 0)
		object.classPath = "com.barry." + appName;
	else if (len != 0xFFFFFF)
		object.classPath = ParseString(input, len);

	len  = ParseInteger(input);

	object.sourceFile = ParseString(input, len);

	object.unknown05 = ParseInteger(input);
	object.unknown06 = ParseInteger(input);
	object.unknown07 = ParseInteger(input);
	object.unknown08 = ParseInteger(input);

	classList.push_back(object);

	cout << "  name : " << object.className << endl;
	cout << "  path : " << object.classPath << endl;
	cout << "  type : " << hex << object.type << endl;
	cout << "  unknown02 : " << hex << object.unknown02 << endl;
	cout << "  unknown03 : " << hex << object.unknown03 << endl;
	cout << "  id : " << hex << object.id << endl;
	cout << "  source file : " << object.sourceFile << endl;
	cout << "  unknown05 : " << hex << object.unknown05 << endl;
	cout << "  unknown06 : " << hex << object.unknown06 << endl;
	cout << "  unknown07 : " << hex << object.unknown07 << endl;
	cout << "  unknown08 : " << hex << object.unknown08 << endl;
}


void CodInfo::ParseArray(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseArray" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseVoid(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseVoid" << endl;
	cout << "  name : " << str << endl;
}


void CodInfo::ParseDouble(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "JDG::CodInfo::ParseDouble" << endl;
	cout << "  name : " << str << endl;
}

/*
void CodInfo::ParseType2(ifstream &input) {
	uint32_t value;
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Type2 : " << str << endl;

	value = ParseInteger(input);
	value = ParseInteger(input);
	value = ParseInteger(input);
	value = ParseInteger(input);
	value = ParseInteger(input);
	value = ParseInteger(input);
}
*/
} // namespace JDG

} // namespace Barry

