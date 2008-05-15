///
/// \file	pppfilter.cc
///		Data filter class, to morph PPP data into something that
///		the Blackberry / Rogers / ISP can handle.
///		This logic is based partly on XmBlackBerry's
///		gprs_protocol_fix.c program.
///

#include "pppfilter.h"

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// PppFilter class

PppFilter::PppFilter()
	: m_ppp_mode(false)
	, m_last(0x7e)
{
}

const Data& PppFilter::Write(const Data &data)
{
	if( data.GetSize() == 0 )
		return data;	// nothing to do

	const unsigned char *b = data.GetData(), *e = data.GetData() + data.GetSize();

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

	size_t needed = data.GetSize() / 2 * 3 + 4;	// worst case
	unsigned char *buf = m_writeBuf.GetBuffer(needed);
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

	m_writeBuf.ReleaseBuffer(put - buf);
	return m_writeBuf;
}

} // namespace Barry

