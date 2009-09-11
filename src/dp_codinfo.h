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

	std::string GetFullClassName() { return classPath + "." + className; };
};


class BXEXPORT ClassList : public std::vector<ClassEntry>
{
private:
protected:

public:
	void CreateDefaultEntries();
};





class BXEXPORT CodInfo
{
private:
	uint32_t ParseNextHeaderField(std::ifstream &input);
	uint32_t ParseNextTypeField(std::ifstream &input);

	void ParseAppName(std::ifstream &input);
	void ParseUniqueId(std::ifstream &input);

	void ParseBoolean(std::ifstream &input);
	void ParseByte(std::ifstream &input);
	void ParseChar(std::ifstream &input);
	void ParseShort(std::ifstream &input);
	void ParseInt(std::ifstream &input);
	void ParseLong(std::ifstream &input);
	void ParseClass(std::ifstream &input);
	void ParseArray(std::ifstream &input);
	void ParseVoid(std::ifstream &input);
	void ParseDouble(std::ifstream &input);

protected:

public:
	uint32_t uniqueId;
	std::string appName;
	ClassList classList;

	bool LoadDebugFile(const char *filename);

	void ParseHeaderSection(std::ifstream &input);
	void ParseTypeSection(std::ifstream &input);
	void ParseResourceSection(std::ifstream &input);

	uint32_t GetUniqueId();
	std::string GetAppName();
};


BXEXPORT void SearchDebugFile(DebugFileList &list);
BXEXPORT bool LoadDebugInfo(DebugFileList &list, const char *filename, CodInfo &info);
BXEXPORT bool LoadDebugInfo(DebugFileList &list, const uint32_t uniqueId, const std::string module, CodInfo &info);


} // namespace JDG

} // namespace Barry


#endif

