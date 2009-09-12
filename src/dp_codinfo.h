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
	typedef std::vector<DebugFileEntry>		base_type;
	typedef base_type::iterator			iterator;
	typedef base_type::const_iterator		const_iterator;

public:
	void AddElement(uint32_t uniqueid, const std::string &appname, const std::string &filename);
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
public:
	typedef std::vector<ClassEntry>			base_type;
	typedef base_type::iterator			iterator;
	typedef base_type::const_iterator		const_iterator;

public:
	void CreateDefaultEntries();
};





class BXEXPORT CodInfo
{
private:
	uint32_t ParseNextHeaderField(std::istream &input);
	uint32_t ParseNextTypeField(std::istream &input);

	void ParseAppName(std::istream &input);
	void ParseUniqueId(std::istream &input);

	void ParseBoolean(std::istream &input);
	void ParseByte(std::istream &input);
	void ParseChar(std::istream &input);
	void ParseShort(std::istream &input);
	void ParseInt(std::istream &input);
	void ParseLong(std::istream &input);
	void ParseClass(std::istream &input);
	void ParseArray(std::istream &input);
	void ParseVoid(std::istream &input);
	void ParseDouble(std::istream &input);

protected:

public:
	uint32_t uniqueId;
	std::string appName;
	ClassList classList;

	bool LoadDebugFile(const char *filename);

	void ParseHeaderSection(std::istream &input);
	void ParseTypeSection(std::istream &input);
	void ParseResourceSection(std::istream &input);

	uint32_t GetUniqueId();
	std::string GetAppName();
};


BXEXPORT void SearchDebugFile(DebugFileList &list);
BXEXPORT bool LoadDebugInfo(const DebugFileList &list, const char *filename, CodInfo &info);
BXEXPORT bool LoadDebugInfo(const DebugFileList &list, const uint32_t uniqueId, const std::string module, CodInfo &info);


} // namespace JDG

} // namespace Barry


#endif

