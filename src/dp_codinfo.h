///
/// \file	dp_codinfo.h
///		Debug file parsing
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

#ifndef __BARRYJDG_CODINFO_H__
#define __BARRYJDG_CODINFO_H__


#include "dll.h"
#include <iosfwd>
#include <string>
#include <vector>


#define COD_DEBUG_APPNAME_HEADERFIELD	0x0
#define COD_DEBUG_UNIQUEID_HEADERFIELD	0x8

#define COD_DEBUG_NONE_FIELD			0x0
#define COD_DEBUG_BOOLEAN_FIELD			0x1
#define COD_DEBUG_BYTE_FIELD			0x2
#define COD_DEBUG_CHAR_FIELD			0x3
#define COD_DEBUG_SHORT_FIELD			0x4
#define COD_DEBUG_INT_FIELD			0x5
#define COD_DEBUG_LONG_FIELD			0x6
#define COD_DEBUG_CLASS_FIELD			0x7
#define COD_DEBUG_ARRAY_FIELD			0x8
#define COD_DEBUG_VOID_FIELD			0xA
#define COD_DEBUG_DOUBLE_FIELD			0xC


namespace Barry {

namespace JDG {


class BXEXPORT DebugFileEntry
{
private:
protected:

public:
	std::string fileName;
	std::string appName;
	uint32_t uniqueId;

	void Dump(std::ostream &os) const;
};


class BXEXPORT DebugFileList : public std::vector<DebugFileEntry>
{
public:
	void AddElement(uint32_t uniqueid, std::string appname, std::string filename);
	void Dump(std::ostream &os) const;	
};
inline std::ostream& operator<<(std::ostream &os, const DebugFileList &list) {
	list.Dump(os);
	return os;
}


class BXEXPORT ClassEntry
{
private:
protected:

public:
	// For JDB
	int index;

	// Read from the ".debug" file
	std::string className;
	std::string classPath;
	std::string sourceFile;

	uint32_t type;
	uint32_t unknown02;
	uint32_t unknown03;
	uint32_t id;
	uint32_t unknown05;
	uint32_t unknown06;
	uint32_t unknown07;
	uint32_t unknown08;

	std::string getFullClassName() { return classPath + "." + className; };
};


class BXEXPORT ClassList : public std::vector<ClassEntry>
{
private:
protected:

public:
	void createDefaultEntries();
};





class BXEXPORT CodInfo
{
private:
	uint32_t parseNextHeaderField(std::ifstream &input);
	uint32_t parseNextTypeField(std::ifstream &input);

	void parseAppName(std::ifstream &input);
	void parseUniqueId(std::ifstream &input);

	void parseBoolean(std::ifstream &input);
	void parseByte(std::ifstream &input);
	void parseChar(std::ifstream &input);
	void parseShort(std::ifstream &input);
	void parseInt(std::ifstream &input);
	void parseLong(std::ifstream &input);
	void parseClass(std::ifstream &input);
	void parseArray(std::ifstream &input);
	void parseVoid(std::ifstream &input);
	void parseDouble(std::ifstream &input);

protected:

public:
	uint32_t uniqueId;
	std::string appName;
	ClassList classList;

	bool loadDebugFile(const char *filename);

	void parseHeaderSection(std::ifstream &input);
	void parseTypeSection(std::ifstream &input);

	uint32_t getUniqueId();
	std::string getAppName();
};


BXEXPORT void searchDebugFile(DebugFileList &list);
BXEXPORT bool loadDebugInfo(DebugFileList &list, const char *filename, CodInfo &info);
BXEXPORT bool loadDebugInfo(DebugFileList &list, const uint32_t uniqueId, const std::string module, CodInfo &info);


} // namespace JDG

} // namespace Barry


#endif

