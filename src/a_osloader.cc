///
/// \file	a_osloader.cc
///		OS files parser (multi ALX files parser)
///

/*
    Copyright (C) 2010, Nicolas VIVIEN
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <fstream>
#include <algorithm>

#include <sys/types.h>
#include <dirent.h>

#include "a_osloader.h"
#include "a_alxparser.h"


namespace Barry {

namespace ALX {


OSLoader::OSLoader(void) 
{
}


OSLoader::~OSLoader(void)
{
}


void OSLoader::load(const std::string& pathname)
{
#define ALX_FILE_EXT	".alx"

	int offset;

	DIR *path;
	struct dirent *entry;

	std::string alxfile;
	const std::string ext = ALX_FILE_EXT;

	// At first, we have to read platform properties...
	alxfile = pathname + "/Platform.alx";
	loadALXFile(alxfile, false);

	// Then, we can read all ALX files
	path = opendir(pathname.c_str());

	while ((entry = readdir(path)) != NULL) {
		alxfile = entry->d_name;

		if (alxfile.length() < ext.length())
			continue;

		offset = alxfile.length() - ext.length();

		// Not supported...
		if (alxfile == "BlackBerry.alx")
			continue;

		// Ignore all files except ".alx" files
		if (alxfile.substr(offset, ext.length()) != ALX_FILE_EXT)
			continue;

		loadALXFile(pathname + "/" + alxfile, true);
	}

	closedir(path);
}


void OSLoader::loadALXFile(const std::string& alxfile, const bool enable)
{
	std::ifstream file(alxfile.c_str());

	ALX::ALXParser parser(*this, file);

	parser.run(enable);

	file.close();
}


void OSLoader::dump(std::ostream &os) const
{
	os << "OS Properties :" << std::endl;

	{
		std::map<std::string, std::string>::const_iterator b = properties.begin(), e = properties.end();

		for (; b != e; b++) {
			os << "  - " << (*b).first << " = " << (*b).second << std::endl;
		}
	}

	os << std::endl;

	os << "SFI File :" << std::endl;
	os << "  " << sfifile << std::endl;
	os << std::endl;

	os << "Applications :" << std::endl;

	{
		std::vector<CODSection *>::const_iterator b = applications.begin(), e = applications.end();
	
		for (; b != e; b++) {
			os << (**b) << std::endl;
		}
	}

	os << "Libraries :" << std::endl;

	{
		std::vector<CODSection *>::const_iterator b = libraries.begin(), e = libraries.end();
	
		for (; b != e; b++) {
			os << (**b) << std::endl;
		}
	}
}


void OSLoader::addProperties(const std::string& property, const std::string& value) 
{
	properties[property] = value;

	if (property == "JVMLevel")
		properties["Java"] = value;
}

	
void OSLoader::addProperties(const xmlpp::SaxParser::AttributeList& attrs)
{
	for (xmlpp::SaxParser::AttributeList::const_iterator iter = attrs.begin(); iter != attrs.end(); ++iter) {
		std::string attribut(iter->name);
		std::string value(iter->value);

		addProperties(attribut, value);
	}
}


void OSLoader::setSFIFile(const std::string& name)
{
	sfifile = name;
}


bool OSLoader::isSupported(const xmlpp::SaxParser::AttributeList& attrs)
{
	if (properties.empty())
		return false;

	for (xmlpp::SaxParser::AttributeList::const_iterator iter = attrs.begin(); iter != attrs.end(); ++iter) {
		std::string attribut(iter->name);
		std::string value(iter->value);

		std::string s = properties[attribut];

		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);

		if (value[0] == '~') {
			value = value.substr(1, value.length()-1);

			if (s == value)
				return false;
		}
		else {
			if (s != value)
				return false;
		}
	}

	return true;
}


void OSLoader::addApplication(CODSection *app)
{
	applications.push_back(app);
}


void OSLoader::addLibrary(CODSection *lib)
{
	libraries.push_back(lib);
}


} // namespace ALX

} // namespace Barry

