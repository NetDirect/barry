/** 
 * @file codinfo.cc
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
#include <iomanip>

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "parser.h"
#include "codinfo.h"


using namespace std;


namespace Barry {

namespace JDG {


// Public API
//------------

#define DEBUG_FILE_EXT		".debug"


void searchDebugFile(JDGDebugFileList &list)
{
	DIR *path;
	struct dirent *entry;

	path = opendir(".");

	while (entry = readdir(path)) {
		int offset;

		if (strlen(entry->d_name) < strlen(DEBUG_FILE_EXT))
			continue;

		offset = strlen(entry->d_name) - strlen(DEBUG_FILE_EXT);

		if (!strcmp(entry->d_name + offset, DEBUG_FILE_EXT)) {
			ifstream file(entry->d_name);

			JDGCodInfo info;

			// Parse header section
			info.parseHeaderSection(file);

			// Add element to list
			list.AddElement(info.getUniqueId(), info.getAppName(), entry->d_name);
		}
	}

	closedir(path);
}


bool loadDebugInfo(JDGDebugFileList &list, const char *filename, JDGCodInfo &info)
{
	if (filename == NULL)
		return false;

	vector<JDGDebugFileEntry>::iterator b = list.begin();

	for( ; b != list.end(); b++ ) {
		JDGDebugFileEntry entry = (*b);

		if (entry.fileName == string(filename)) {
			info.loadDebugFile(filename);
			return true;
		}
	}

	return false;
}


bool loadDebugInfo(JDGDebugFileList &list, const uint32_t uniqueId, const std::string module, JDGCodInfo &info)
{
	vector<JDGDebugFileEntry>::iterator b = list.begin();

	for( ; b != list.end(); b++ ) {
		JDGDebugFileEntry entry = (*b);

		if ((entry.uniqueId == uniqueId) && (entry.appName == module)) {
			info.loadDebugFile(entry.fileName.c_str());
			return true;
		}
	}

	return false;
}


// JDGDebugFileList class
//------------------------

void JDGDebugFileList::AddElement(uint32_t uniqueid, std::string appname, std::string filename)
{
	JDGDebugFileEntry entry;

	entry.uniqueId = uniqueid;
	entry.appName = appname;
	entry.fileName = filename;

	push_back(entry);
}


void JDGDebugFileList::Dump(std::ostream &os) const
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


void JDGDebugFileEntry::Dump(std::ostream &os) const
{
	os << " 0x" << setfill('0') << setw(8) << hex << uniqueId << " |";
	os << " " << appName << setfill(' ') << setw(24) << " |";
	os << " " << fileName << endl;
}


// JDGCodInfo class
//------------------------

bool JDGCodInfo::loadDebugFile(const char *filename)
{
	if (filename == NULL)
		return false;

	ifstream file(filename);

	// Parse header file
	parseHeaderSection(file);

	// Parse type area zone
	parseTypeSection(file);

	return true;
}


uint32_t JDGCodInfo::getUniqueId() 
{
	return uniqueId;
}


string JDGCodInfo::getAppName() 
{
	return appName;
}


// Private API - Section parsing
//-------------------------------

void JDGCodInfo::parseHeaderSection(ifstream &input) 
{
	uint32_t type;

	type = parseNextHeaderField(input);

	if (type != COD_DEBUG_UNIQUEID_HEADERFIELD)
		return;

	type = parseNextHeaderField(input);

	if (type != COD_DEBUG_APPNAME_HEADERFIELD)
		return;
}


void JDGCodInfo::parseTypeSection(ifstream &input) 
{
	uint32_t type;
	uint32_t count;
	uint32_t nbr, check;

	// Read number of declared type content into this section
	nbr = ParseInteger(input);

	// Read each object
	count = 0;

	while (!input.eof()) {
		type = parseNextTypeField(input);

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


uint32_t JDGCodInfo::parseNextHeaderField(ifstream &input) 
{
	uint32_t type = ParseInteger(input);

	switch (type) {
	case COD_DEBUG_UNIQUEID_HEADERFIELD:
		parseUniqueId(input);
		break;

	case COD_DEBUG_APPNAME_HEADERFIELD:
		parseAppName(input);
		break;

	default:
		type = 0xFFFFFFFF;
	}

	return type;
}


uint32_t JDGCodInfo::parseNextTypeField(ifstream &input) 
{
	uint32_t type = ParseInteger(input);

	switch (type) {
	case COD_DEBUG_NONE_FIELD:
		break;

	case COD_DEBUG_BOOLEAN_FIELD:
		parseBoolean(input);
		break;

	case COD_DEBUG_BYTE_FIELD:
		parseByte(input);
		break;

	case COD_DEBUG_CHAR_FIELD:
		parseChar(input);
		break;

	case COD_DEBUG_SHORT_FIELD:
		parseShort(input);
		break;

	case COD_DEBUG_INT_FIELD:
		parseInt(input);
		break;

	case COD_DEBUG_LONG_FIELD:
		parseLong(input);
		break;

	case COD_DEBUG_CLASS_FIELD:
		parseClass(input);
		break;

	case COD_DEBUG_ARRAY_FIELD:
		parseArray(input);
		break;

	case COD_DEBUG_VOID_FIELD:
		parseVoid(input);
		break;

	case COD_DEBUG_DOUBLE_FIELD:
		parseDouble(input);
		break;

	default:
		cout << "Type unknown ! " << hex << type << endl;
		type = 0xFFFFFFFF;
	}

	return type;
}


void JDGCodInfo::parseUniqueId(ifstream &input) 
{
	uniqueId = ParseInteger(input);
}


void JDGCodInfo::parseAppName(ifstream &input) 
{
	uint32_t len = ParseInteger(input);

	appName = ParseString(input, len);
}


void JDGCodInfo::parseBoolean(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Boolean name : " << str << endl;
}


void JDGCodInfo::parseByte(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Byte name : " << str << endl;
}


void JDGCodInfo::parseChar(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Char name : " << str << endl;
}


void JDGCodInfo::parseShort(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Short name : " << str << endl;
}


void JDGCodInfo::parseInt(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Int name : " << str << endl;
}


void JDGCodInfo::parseLong(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Long name : " << str << endl;
}


void JDGCodInfo::parseClass(ifstream &input) 
{
	uint32_t len;

	JDGClassEntry object;

	cout << "JDGCodInfo::parseClass" << endl;

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

	cout << "class added !" << endl;
}


void JDGCodInfo::parseArray(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Array name : " << str << endl;
}


void JDGCodInfo::parseVoid(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Void : " << str << endl;
}


void JDGCodInfo::parseDouble(ifstream &input) 
{
	uint32_t len  = ParseInteger(input);

	string str = ParseString(input, len);

	cout << "Double : " << str << endl;
}

/*
void JDGCodInfo::parseType2(ifstream &input) {
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

