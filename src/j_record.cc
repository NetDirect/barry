///
/// \file	j_record.cc
///		Internal record manipulation functions for JDWP classes
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

#include "record-internal.h"
#include "data.h"
#include "protostructs.h"
#include <string.h>

using namespace Barry::Protocol;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// JDWField builder helper functions

void AddJDWByte(Data &data, size_t &size, const uint8_t value)
{
	size_t fieldsize = sizeof(uint8_t);
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;

	uint8_t *field = (uint8_t *) pd;

	*field = value;

	size += sizeof(uint8_t);
}


void AddJDWInt(Data &data, size_t &size, const uint32_t value)
{
	size_t fieldsize = sizeof(uint32_t);
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;

	uint32_t *field = (uint32_t *) pd;

	*field = value;

	size += sizeof(uint32_t);
}


void AddJDWChar(Data &data, size_t &size, const void *buf, size_t bufsize)
{
	size_t fieldsize = JDWP_FIELD_HEADER_SIZE + bufsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;

	JDWField *field = (JDWField *) pd;

	field->size = be_htobl(bufsize);
	memcpy(field->u.raw, buf, bufsize);

	size += fieldsize;
}


void AddJDWString(Data &data, size_t &size, const std::string &str)
{
	AddJDWChar(data, size, str.c_str(), str.size());
}

} // namespace Barry

