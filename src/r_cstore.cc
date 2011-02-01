///
/// \file	r_cstore.cc
///		Blackberry database record parser class for
///		Content Store records.
///

/*
    Copyright (C) 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "r_cstore.h"
#include "record-internal.h"
#include "data.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// ContentStore class

// ContentStore field codes
#define CSFC_FILENAME		0x01	// may not always be a complete file,but
					// a folder name as well
#define CSFC_FOLDER_FLAG	0x05
#define CSFC_FILE_DESCRIPTOR	0x06
#define CSFC_FILE_CONTENT	0x07


#define MAX_CONTENT_BLOCK_SIZE	0xfffe

ContentStore::ContentStore()
{
	Clear();
}

ContentStore::~ContentStore()
{
}

const unsigned char* ContentStore::ParseField(const unsigned char *begin,
					 const unsigned char *end,
					 const IConverter *ic)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )	// if field has no size, something's up
		return begin;

	switch( field->type )
	{
	case CSFC_FILENAME:
		Filename = ParseFieldString(field);
		return begin;

	case CSFC_FOLDER_FLAG:
		FolderFlag = false;
		{
			// the CSFC_FOLDER_FLAG field seems to always
			// contain the string "folder".. so check for it
			string s = ParseFieldString(field);
			if( s == "folder" ) {
				FolderFlag = true;
			}
		}
		return begin;

	case CSFC_FILE_CONTENT:
		if( FileSize ) {
			// size already received, append data to FileContent
			FileContent.append((const char*)field->u.raw,
				btohs(field->size));
		}
		else {
			FileSize = btohll(field->u.uint64);
		}
		return begin;

	case CSFC_FILE_DESCRIPTOR:
		// need to parse this further, but until then, just
		// store it as a chunk of data
		FileDescriptor.assign((const char*)field->u.raw,
			btohs(field->size));
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void ContentStore::ParseHeader(const Data &data, size_t &offset)
{
	// no header to parse
}

void ContentStore::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void ContentStore::BuildHeader(Data &data, size_t &offset) const
{
}

void ContentStore::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	data.Zap();

	if( !Filename.size() )
		throw BadData("Content Store must have a name.");

	if( !FolderFlag && !FileContent.size() )
		throw BadData("Content Store item without any data.");

	// Filename
	BuildField(data, offset, CSFC_FILENAME, Filename);

	// Folder?
	if( FolderFlag ) {
		BuildField(data, offset, CSFC_FOLDER_FLAG, string("folder"));
	}
	else {
		// write file descriptor first
		BuildField(data, offset, CSFC_FILE_DESCRIPTOR,
			FileDescriptor.data(), FileDescriptor.size());

		// a normal file... the file content is given:
		//	64 bit size
		//	content in blocks of 0xfffe bytes until done
		// all with the same ID

		// force the size to actual, and write it first
		uint64_t RealSize = FileContent.size();
		BuildField(data, offset, CSFC_FILE_CONTENT, RealSize);

		// write data in blocks of 0xfffe bytes
		for( size_t foff = 0; foff < FileContent.size(); ) {
			size_t blocksize = FileContent.size() - foff;
			if( blocksize > MAX_CONTENT_BLOCK_SIZE )
				blocksize = MAX_CONTENT_BLOCK_SIZE;
			BuildField(data, offset, CSFC_FILE_CONTENT,
				FileContent.data() + foff, blocksize);

			// advance
			foff += blocksize;
		}
	}

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, *ub);
	}

	data.ReleaseBuffer(offset);
}

void ContentStore::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	Filename.clear();
	FolderFlag = false;
	FileContent.clear();
	FileDescriptor.clear();

	Unknowns.clear();

	// internal variables
	FileSize = 0;
}

std::string ContentStore::GetDescription() const
{
	return Filename;
}

void ContentStore::Dump(std::ostream &os) const
{
	ios::fmtflags oldflags = os.setf(ios::left);
	char fill = os.fill(' ');

	os << "ContentStore: 0x" << hex << RecordId
		<< " (" << (unsigned int)RecType << ")\n";

	os << "       Filename: " << Filename << endl;
	os << "         Folder: " << (FolderFlag ? "yes" : "no") << endl;
	os << "        BB Size: " << dec << FileSize << endl;
	os << "    Actual Size: " << FileContent.size() << endl;
	os << "     Descriptor:\n"
		<< Data(FileDescriptor.data(), FileDescriptor.size()) << endl;
	os << "        Content:\n"
		<< Data(FileContent.data(), FileContent.size()) << endl;

	// and finally print unknowns
	os << Unknowns;

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
}

bool ContentStore::operator<(const ContentStore &other) const
{
	return RecordId < other.RecordId;
}

} // namespace Barry

