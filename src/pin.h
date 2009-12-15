///
/// \file	pin.h
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

#ifndef __BARRY_PIN_H__
#define __BARRY_PIN_H__

#include "dll.h"
#include <stdint.h>
#include <string>

namespace Barry {

class BXEXPORT Pin
{
	uint32_t pin;

public:
	Pin(uint32_t pin__ = 0) : pin(pin__) {}

	bool valid() const { return pin != 0; }

	std::string str() const;
	uint32_t value() const { return pin; }

	Pin& operator=(uint32_t p) { pin = p; return *this; }

	bool operator==(uint32_t rhs) const { return pin == rhs; }
	bool operator==(const Pin &rhs) const { return pin == rhs.pin; }

	bool operator!=(uint32_t rhs) const { return pin != rhs; }
	bool operator!=(const Pin &rhs) const { return pin != rhs.pin; }
};

// no ostream operator, since we want to encourage users to call str()...
// but istream may be useful
BXEXPORT std::istream& operator>>(std::istream &is, Pin &pin);

} // namespace Barry

#endif

