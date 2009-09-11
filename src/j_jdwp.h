///
/// \file	jdwp.h
///		JDWP classes
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

#ifndef __BARRYJDWP_JDWP_H__
#define __BARRYJDWP_JDWP_H__

#include "error.h"

namespace Barry {

// forward declarations
class Data;

namespace JDWP {

/// \addtogroup exceptions
/// @{

/// Thrown on low level JDWP errors.
class Error : public Barry::Error
{
	int m_errcode;

public:
	Error(const std::string &str);
	Error(int errcode, const std::string &str);

	// can return 0 in some case, if unknown error code
	int errcode() const { return m_errcode; }
};

class Timeout : public Error
{
public:
	Timeout(const std::string &str) : Error(str) {}
	Timeout(int errcode, const std::string &str)
		: Error(errcode, str) {}
};

/// @}



class JDWP {
protected:

private:
	int m_lasterror;

public:
	JDWP();
	~JDWP();

	bool Read(int socket, Barry::Data &data, int timeout = -1);
	bool Write(int socket, const Barry::Data &data, int timeout = -1);
	bool Write(int socket, const void *data, size_t size, int timeout = -1);
};

}} // namespace Barry::JDWP

#endif

