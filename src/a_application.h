///
/// \file	a_application.h
///		ALX Application class based on CODSection class
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

#ifndef __BARRY_A_APPLICATION_H__
#define __BARRY_A_APPLICATION_H__

#include <vector>

#include "dll.h"
#include "a_codsection.h"


namespace Barry {

namespace ALX {


class BXEXPORT Application : public CODSection
{
public:
	Application(void);
	Application(const xmlpp::SaxParser::AttributeList& attrs);
	virtual ~Application(void);

	virtual void Dump(std::ostream &os) const;
};


inline std::ostream& operator<<(std::ostream& os, const Application& app)
{
	app.Dump(os);
	return os;
}


} // namespace ALX

} // namespace Barry

#endif


