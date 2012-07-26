///
/// \file	r_command.cc
///		Internal implementation of CommandTable parser class
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "record.h"
#include "record-internal.h"
#include "data.h"
#include <sstream>
#include <iomanip>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// CommandTable class

CommandTable::CommandTable()
{
}

CommandTable::~CommandTable()
{
}

const unsigned char* CommandTable::ParseField(const unsigned char *begin,
					      const unsigned char *end)
{
	// check if there is enough data for a header
	const unsigned char *headend = begin + sizeof(CommandTableField);
	if( headend > end )
		return headend;

	const CommandTableField *field = (const CommandTableField *) begin;

	// advance and check size
	begin += COMMAND_FIELD_HEADER_SIZE + field->size;	// size is byte
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	Command command;
	command.Code = field->code;
	command.Name.assign((const char *)field->name, field->size);
	Commands.push_back(command);
	return begin;
}

void CommandTable::Parse(const Data &data, size_t offset)
{
	if( offset >= data.GetSize() )
		return;

	const unsigned char *begin = data.GetData() + offset;
	const unsigned char *end = data.GetData() + data.GetSize();

	while( begin < end )
		begin = ParseField(begin, end);
}

void CommandTable::Clear()
{
	Commands.clear();
}

unsigned int CommandTable::GetCommand(const std::string &name) const
{
	CommandArrayType::const_iterator b = Commands.begin();
	for( ; b != Commands.end(); b++ )
		if( b->Name == name )
			return b->Code;
	return 0;
}

void CommandTable::Dump(std::ostream &os) const
{
	CommandArrayType::const_iterator b = Commands.begin();
	os << _("Command table:\n");
	for( ; b != Commands.end(); b++ ) {
		os << _("    Command: ") << "0x" << setbase(16) << b->Code
		   << " '" << b->Name << "'\n";
	}
}


} // namespace Barry

