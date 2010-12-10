/**
 * @file dp_parser.h
 * @author Nicolas VIVIEN
 * @date 2009-08-01
 *
 * @note CopyRight Nicolas VIVIEN
 *
 * @brief COD debug file parser
 *
 * @par Modifications
 *   - 2009/08/01 : N. VIVIEN
 *     - First release
 *
 * @par Licences
 *   Copyright (C) 2009-2010, Nicolas VIVIEN
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *   See the GNU General Public License in the COPYING file at the
 *   root directory of this project for more details.
 */


#ifndef __BARRYJDG_PARSER_H__
#define __BARRYJDG_PARSER_H__

#include <iosfwd>
#include <string>
#include <stdint.h>

namespace Barry {

namespace JDG {

std::string ParseString(std::istream &input, const int length);
uint32_t ParseInteger(std::istream &input);

} // namespace JDG

} // namespace Barry

#endif

