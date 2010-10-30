///
/// \file	a_codsection.h
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

#ifndef __BARRY_A_CODSECTION_H__
#define __BARRY_A_CODSECTION_H__

#include <vector>

#include "dll.h"
#include "a_common.h"


namespace Barry {

namespace ALX {


class BXEXPORT CODSection {
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

	virtual void dump(std::ostream &os) const = 0;

	virtual void setID(const std::string& id);
	virtual void setName(const std::string& name);
	virtual void setDescription(const std::string& description);
	virtual void setVersion(const std::string& version);
	virtual void setVendor(const std::string& vendor);
	virtual void setCopyright(const std::string& copyright);
	virtual void setDirectory(const std::string& directory);
	virtual void setRequired(const std::string& required);
	virtual void addFiles(const std::string& files);
	virtual void addFile(const std::string& files);
};


inline std::ostream& operator<<(std::ostream& os, const CODSection& cod)
{
	cod.dump(os);
	return os;
}


} // namespace ALX

} // namespace Barry

#endif


