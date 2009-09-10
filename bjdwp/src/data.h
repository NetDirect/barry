///
/// \file	data.h
///		Auxilliary functions for the Data class
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

#ifndef __BARRYJDWP_DATA_H__
#define __BARRYJDWP_DATA_H__


namespace JDWP {


void AddDataByte(Barry::Data &data, size_t &size, const uint8_t value);
void AddDataInt(Barry::Data &data, size_t &size, const uint32_t value);
void AddDataChar(Barry::Data &data, size_t &size, const void *buf, size_t bufsize);
void AddDataString(Barry::Data &data, size_t &size, const std::string &str);

}

#endif

