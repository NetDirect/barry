///
/// \file	a_codsection.cc
///		COD structure for the ALX file parser
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
#include <sstream>

#include "a_codsection.h"


namespace Barry {

namespace {

std::string trim(const std::string &str) {
	int i;
	int start, end;

	std::string s;

	const int len = str.size();

	// ltrim
	for (i=0; (((str[i]==' ') || (str[i]=='\t') || (str[i]=='\r') || (str[i]=='\n')) && (i<len)); i++);
    start = i;

    // rtrim
	for (i=len-1; (((str[i]==' ') || (str[i]=='\t') || (str[i]=='\r') || (str[i]=='\n')) && (i>=0)); i--);
	end = i+1;

    s = str.substr(start, end-start);

    return s;
}

}


namespace ALX {


CODSection::CODSection(void)
{
	isRequired = false;
}


CODSection::CODSection(const xmlpp::SaxParser::AttributeList& attrs)
{
	isRequired = false;

	for (xmlpp::SaxParser::AttributeList::const_iterator iter = attrs.begin(); iter != attrs.end(); ++iter) {
		std::string attribut(iter->name);
		std::string value(iter->value);

		if (attribut == "id")
			SetID(value);
	}
}


CODSection::~CODSection(void)
{
	isRequired = false;
}


void CODSection::SetID(const std::string& id)
{
	this->id = id;
}


void CODSection::SetName(const std::string& name)
{
	this->name = trim(name);
}


void CODSection::SetDescription(const std::string& description)
{
	this->description = trim(description);
}


void CODSection::SetVersion(const std::string& version)
{
	this->version = trim(version);
}


void CODSection::SetVendor(const std::string& vendor)
{
	this->vendor = trim(vendor);
}


void CODSection::SetCopyright(const std::string& copyright)
{
	this->copyright = trim(copyright);
}


void CODSection::SetDirectory(const std::string& directory)
{
	this->directory = trim(directory);
}


void CODSection::SetRequired(const std::string& required)
{
	std::string s = trim(required);

	if (s == "true")
		isRequired = true;
	else
		isRequired = false;
}


void CODSection::AddFiles(const std::string& files) 
{
	std::string file;
	std::istringstream iss(files);

	while( std::getline(iss, file) ) {
		file = trim(file);

		if (file.length() > 0)
			AddFile(file);
	}
}


void CODSection::AddFile(const std::string& file)
{
	codfiles.push_back(file);
}


} // namespace ALX

} // namespace Barry

