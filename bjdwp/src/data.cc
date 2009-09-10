///
/// \file	data.cc
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

#include <string.h>
#include <string>

#include <barry/endian.h>
#include <barry/barry.h>

#include "data.h"


using namespace Barry;


namespace JDWP {


typedef struct {
	uint32_t size;

	union JDWFieldData {
		uint8_t raw[1];
	} __attribute__ ((packed)) u;
} __attribute__ ((packed)) JDWField;

#define JDWP_FIELD_HEADER_SIZE 		(sizeof(JDWField) - sizeof(JDWField::JDWFieldData))


void AddDataByte(Data &data, size_t &size, const uint8_t value)
{
	size_t fieldsize = sizeof(uint8_t);
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;

	uint8_t *field = (uint8_t *) pd;

	*field = value;

	size += sizeof(uint8_t);
}


void AddDataInt(Data &data, size_t &size, const uint32_t value)
{
	size_t fieldsize = sizeof(uint32_t);
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;

	uint32_t *field = (uint32_t *) pd;

	*field = value;

	size += sizeof(uint32_t);
}


void AddDataChar(Data &data, size_t &size, const void *buf, size_t bufsize)
{
	size_t fieldsize = JDWP_FIELD_HEADER_SIZE + bufsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;

	JDWField *field = (JDWField *) pd;

	field->size = be_htobl(bufsize);
	memcpy(field->u.raw, buf, bufsize);

	size += fieldsize;
}


void AddDataString(Data &data, size_t &size, const std::string &str)
{
	AddDataChar(data, size, str.c_str(), str.size());
}

} // namespace JDWP

