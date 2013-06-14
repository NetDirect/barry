///
/// \file	a_codsection.h
///		COD structure for the ALX file parser
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

#ifndef __BARRY_A_CODSECTION_H__
#define __BARRY_A_CODSECTION_H__

#include <vector>

#include "dll.h"
#include "a_common.h"


namespace Barry {

namespace ALX {


class BXEXPORT CODSection
{
protected:
	std::string id;
	std::string name;
	std::string description;
	std::string version;
	std::string vendor;
	std::string copyright;
	std::string directory;

	bool isRequired;
	std::vector<std::string> codfiles;

public:
	CODSection(void);
	CODSection(const xmlpp::SaxParser::AttributeList& attrs);
	virtual ~CODSection(void);

	virtual void Dump(std::ostream &os) const = 0;

	virtual void SetID(const std::string& id);
	virtual void SetName(const std::string& name);
	virtual void SetDescription(const std::string& description);
	virtual void SetVersion(const std::string& version);
	virtual void SetVendor(const std::string& vendor);
	virtual void SetCopyright(const std::string& copyright);
	virtual void SetDirectory(const std::string& directory);
	virtual void SetRequired(const std::string& required);
	virtual void AddFiles(const std::string& files);
	virtual void AddFile(const std::string& files);
};


inline std::ostream& operator<<(std::ostream& os, const CODSection& cod)
{
	cod.Dump(os);
	return os;
}


} // namespace ALX

} // namespace Barry

#endif


