///
/// \file	a_osloader.h
///		OS files parser (multi ALX files parser)
///

/*
    Copyright (C) 2010, Nicolas VIVIEN
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_A_OSLOADER_H__
#define __BARRY_A_OSLOADER_H__

#include <iostream>
#include <vector>
#include <map>

#include "dll.h"
#include "a_application.h"
#include "a_library.h"
#include <tr1/memory>


#define OS_LANG_ENGLISH			""
#define OS_LANG_ARABIC			"0x0001"
#define OS_LANG_CATALAN			"0x0003"
#define OS_LANG_CZECH			"0x0005"
#define OS_LANG_GERMAN			"0x0007"
#define OS_LANG_SPANISH			"0x000a"
#define OS_LANG_FRENCH			"0x000c"
#define OS_LANG_HEBREW			"0x000d"
#define OS_LANG_HUNGARIAN		"0x000e"
#define OS_LANG_ITALIAN			"0x0010"
#define OS_LANG_JAPANESE		"0x0011"
#define OS_LANG_KOREAN			"0x0012"


namespace Barry {

namespace ALX {

class BXEXPORT OSLoader
{
public:
	typedef std::tr1::shared_ptr<CODSection>	CODSectionPtr;
	typedef std::vector<CODSectionPtr>		CODSectionList;
	typedef std::map<std::string, std::string>	PropertyMap;

private:
	std::string sfifile;
	CODSectionList applications;
	CODSectionList libraries;
	PropertyMap properties;

public:
	OSLoader(void);
	~OSLoader(void);

	void Load(const std::string& path);
	/// Load ALX file, and if enable is true, register the applications
	/// and libraries found inside it with this OSLoader class.
	void LoadALXFile(const std::string& alxfile, const bool enable=true);

	void Dump(std::ostream &os) const;

	void AddProperties(const std::string& property, const std::string& value);
	void AddProperties(const xmlpp::SaxParser::AttributeList& attrs);
	void SetSFIFile(const std::string& name);
	bool IsSupported(const xmlpp::SaxParser::AttributeList& attrs);
	void AddApplication(CODSectionPtr app);
	void AddLibrary(CODSectionPtr lib);
};


inline std::ostream& operator<<(std::ostream& os, const OSLoader& osloader)
{
	osloader.Dump(os);
	return os;
}


} // namespace ALX

} // namespace Barry

#endif

