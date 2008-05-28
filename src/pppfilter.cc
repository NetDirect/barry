///
/// \file	pppfilter.cc
///		Data filter class, to morph PPP data into something that
///		the Blackberry / Rogers / ISP can handle.
///		This logic is based partly on XmBlackBerry's
///		gprs_protocol_fix.c program.
///

#include "pppfilter.h"
#include <string.h>

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// PppFilter class

PppFilter::PppFilter()
	: m_ppp_mode(false)
	, m_last(0x7e)
{
}

//
// Filter
//
/// Copy PPP data from src to dest, creating needed space in dest,
/// and inserting missing 0x7e characters wherever they are detected.
/// Starts writing at the start of the dest buffer + destoffset.
///
void PppFilter::Filter(Data &dest, const Data &src, unsigned int destoffset)
{
	const unsigned char *b = src.GetData(), *e = src.GetData() + src.GetSize();
	size_t needed = src.GetSize() / 2 * 3 + 4 + destoffset; // worst case
	unsigned char *buf = dest.GetBuffer(needed) + destoffset;
	unsigned char *put = buf;

	while( b != e ) {
		// if last character was 0x7e, then next one must be,
		// or else we insert it ourselves
		if( m_last == 0x7e ) {
			m_last = 0;
			if( *b != 0x7e )
				*put++ = 0x7e;
			else
				*put++ = *b++;
		}

		// copy all non-0x7e chars verbatim
		while( b != e && *b != 0x7e ) {
			*put++ = *b++;
		}

		if( b != e ) {	// if b!=e then *b == 0x7e and must keep going
			*put++ = *b++;
			m_last = 0x7e;
		}
	}

	dest.ReleaseBuffer(put - buf + destoffset);
}

//
// Write
//
/// If PPP mode has not been detected, just return the data buffer.
/// If in PPP mode, then filter data into internal write buffer,
/// inserting any missing 0x7e characters and return reference
/// to internal write buffer.
///
const Data& PppFilter::Write(const Data &data)
{
	if( data.GetSize() == 0 )
		return data;	// nothing to do

	const unsigned char *b = data.GetData();

	if( !m_ppp_mode ) {
		if( *b == 0x7e ) {
			m_ppp_mode = true;
			// fall through
		}
		else {
			// not in ppp mode yet, so just pass the buffer
			// straight back to the caller
			return data;
		}
	}

	Filter(m_writeBuf, data, 0);
	return m_writeBuf;
}

//
// Write (with prepend)
//
/// Same as Write(data), but makes sure that prepend bytes are available
/// at the beginning of the returned buffer.
/// If not in PPP mode, the extra bytes are still provided.
///
Data& PppFilter::Write(const Data &data, unsigned int prepend)
{
	const unsigned char *b = data.GetData(), *e = data.GetData() + data.GetSize();

	if( !m_ppp_mode ) {
		if( b != e && *b == 0x7e ) {
			m_ppp_mode = true;
			// fall through
		}
		else {
			// make space, copy, return
			unsigned int size = data.GetSize() + prepend;
			unsigned char *buf = m_writeBuf.GetBuffer(size);
			memcpy(&buf[prepend], data.GetData(), data.GetSize());
			m_writeBuf.ReleaseBuffer(size);
			return m_writeBuf;
		}
	}

	Filter(m_writeBuf, data, prepend);
	return m_writeBuf;
}

} // namespace Barry

