///
/// \file	parser.h
///		Virtual parser wrapper
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_PARSER_H__
#define __BARRY_PARSER_H__

#include "data.h"
#include "protocol.h"
#include "debug.h"

// also acts as a null parser
class Parser
{
public:
	Parser() {}
	virtual ~Parser() {}

	virtual bool operator()(const Data &data) { return true; }

	virtual bool GetOperation(const Data &data, unsigned int &operation);
};


template <class Record, class Storage>
class RecordParser : public Parser
{
	Storage &m_store;

public:
	RecordParser(Storage &storage) : m_store(storage) {}

	virtual bool CheckHeaderSize(const Data &data, unsigned int operation)
	{
		size_t recordsize;
		switch( operation )
		{
		case SB_DBOP_GET_RECORDS:
			// using the new protocol
			recordsize = sizeof(Record::ProtocolRecordType);
			break;

		case SB_DBOP_OLD_GET_RECORDS_REPLY:
			// using the old protocol
			recordsize = sizeof(Record::OldProtocolRecordType);
			break;

		default:
			// unknown protocol
			dout("Unknown protocol");
			return false;
		}

		// calculate the full header size, which is a DBACCESS
		// header size, plus the header size of recordsize...
		size_t fullsize = SB_PACKET_DBACCESS_HEADER_SIZE + recordsize - sizeof(Barry::CommonField);

		// return true if header is ok
		return (unsigned int)data.GetSize() > fullsize;
	}

	virtual bool operator()(const Data &data)
	{
		unsigned int operation;
		if( !GetOperation(data, operation) )
			return false;
		if( !CheckHeaderSize(data, operation) )
			return false;

		Record rec;
		rec.Parse(data, operation);
		m_store(rec);
		return true;
	}
};

#endif

