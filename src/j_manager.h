///
/// \file	j_manager.h
///		Application management classes
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

#ifndef __BARRYJDWP_MANAGER_H__
#define __BARRYJDWP_MANAGER_H__

#include "dll.h"
#include "dp_codinfo.h"
#include <stdint.h>
#include <string>
#include <map>

namespace Barry { namespace JDWP {

class BXEXPORT JDWAppInfo
{
private:
protected:

public:
	uint32_t uniqueId;
	std::string appName;

	Barry::JDG::ClassList classList;

	void Load(Barry::JDG::CodInfo &info);
};


class BXEXPORT JDWAppList : public std::map<uint32_t, JDWAppInfo>
{
};


}} // namespace Barry::JDWP


#endif

