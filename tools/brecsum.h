///
/// \file	brecsum.h
///		The Parser class for brecsum.
///

/*
    Copyright (C) 2008-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_TOOLS_BRECSUM_H__
#define __BARRY_TOOLS_BRECSUM_H__

class ChecksumParser : public Barry::Parser
{
	bool m_IncludeIds;
	Barry::SHA_CTX m_ctx;

public:
	explicit ChecksumParser(bool IncludeIds)
		: m_IncludeIds(IncludeIds)
	{}

	virtual void ParseRecord(const Barry::DBData &data,
				 const Barry::IConverter *ic)
	{
		using namespace std;
		using namespace Barry;

		SHA1_Init(&m_ctx);

		if( m_IncludeIds ) {
			SHA1_Update(&m_ctx, data.GetDBName().c_str(),
				data.GetDBName().size());

			uint8_t recType = data.GetRecType();
			SHA1_Update(&m_ctx, &recType, sizeof(recType));

			uint32_t uniqueId = data.GetUniqueId();
			SHA1_Update(&m_ctx, &uniqueId, sizeof(uniqueId));
		}

		int len = data.GetData().GetSize() - data.GetOffset();
		SHA1_Update(&m_ctx,
			data.GetData().GetData() + data.GetOffset(), len);

		unsigned char sha1[SHA_DIGEST_LENGTH];
		SHA1_Final(sha1, &m_ctx);

		for( int i = 0; i < SHA_DIGEST_LENGTH; i++ ) {
			cout << hex << setfill('0') << setw(2)
				<< (unsigned int) sha1[i];
		}
		cout << endl;
	}
};

#endif

