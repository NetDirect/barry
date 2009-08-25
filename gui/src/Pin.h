///
/// \file	Pin.h
///		class for device PIN notation
///

/*
    Copyright (C) 2007-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_PIN_H__
#define __BARRYBACKUP_PIN_H__

#include <stdint.h>
#include <string>

class Pin
{
	uint32_t pin;

public:
	Pin(uint32_t pin__ = 0) : pin(pin__) {}

	std::string str();
	bool operator==(uint32_t rhs) { return pin == rhs; }
};

#endif
