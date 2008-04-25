///
/// \file	m_ipmodem.cc
///		Mode class for GPRS modem mode (using endpoints on
///		modern devices)
///

#include "m_ipmodem.h"
#include "controller.h"
#include "data.h"
#include "debug.h"
#include <sstream>

namespace Barry { namespace Mode {

const char special_flag[] = { 0x78, 0x56, 0x34, 0x12 };	// 0x12345678

//////////////////////////////////////////////////////////////////////////////
// Mode::IpModem class

IpModem::IpModem(Controller &con,
		DeviceDataCallback callback,
		void *callback_context)
	: m_con(con)
	, m_dev(con.m_dev)
	, m_continue_reading(false)
	, m_ppp_mode(false)
	, m_last(0x7e)
	, m_callback(callback)
	, m_callback_context(callback_context)
{
}

IpModem::~IpModem()
{
	// thread running?
	if( m_continue_reading ) {
		m_continue_reading = false;
		pthread_join(m_modem_read_thread, NULL);
	}
}


//////////////////////////////////////////////////////////////////////////////
// protected API / static functions

void *IpModem::DataReadThread(void *userptr)
{
	IpModem *ipmodem = (IpModem*) userptr;

	int read_ep = ipmodem->m_con.GetProbeResult().m_epModem.read;
	Data data;

	while( ipmodem->m_continue_reading ) {

		try {

			ipmodem->m_dev.BulkRead(read_ep, data, 5000);

			// is it a special code?
			if( data.GetSize() > 4 &&
			    memcmp(data.GetData() + data.GetSize() - 4, special_flag, sizeof(special_flag)) == 0 ) {
				// log, then drop it on the floor for now
				ddout("IPModem special packet:\n" << data);
				continue;
			}

			// call callback if available
			if( ipmodem->m_callback ) {
				(*ipmodem->m_callback)(ipmodem->m_callback_context,
					data.GetData(),
					data.GetSize());
			}
//			else {
//				// append data to readCache
//				FIXME;
//			}

		}
		catch( Usb::Timeout &to ) {
			// do nothing on timeouts
			ddout("Timeout in DataReadThread!");
		}
		catch( std::exception &e ) {
			eout("Exception in IpModem::DataReadThread: " << e.what());
		}
		catch( ... ) {
			eout("Unknown exception in IpModem::DataReadThread, ignoring!");
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// public API

void IpModem::Open()
{
	// check that we have endpoints for the modem
	const Usb::EndpointPair &pair = m_con.GetProbeResult().m_epModem;
	if( !pair.IsComplete() ) {
		std::ostringstream oss;
		oss << "IP Modem not supported by this device: "
			<< "read: " << std::hex << (unsigned int) pair.read
			<< " write: " << std::hex << (unsigned int) pair.write
			<< " type: " << std::hex << (unsigned int) pair.type;
		eout(oss.str());
		throw Barry::Error(oss.str());
	}

	// clear halt when starting out
	m_dev.ClearHalt(pair.read);
	m_dev.ClearHalt(pair.write);

	// spawn read thread
	m_continue_reading = true;
	int ret = pthread_create(&m_modem_read_thread, NULL, &IpModem::DataReadThread, this);
	if( ret ) {
		m_continue_reading = false;
		throw Barry::ErrnoError("IpModem:: Error creating USB read thread.", ret);
	}

//	const char start[] = { 0x01, 0, 0, 0, 1, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };
	const char start[] = { 0x01, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };
	Data block(start, sizeof(start));
	Write(block);
}

void IpModem::Write(const Data &data, int timeout)
{
	if( data.GetSize() == 0 )
		return;	// nothing to do

//	m_dev.ClearHalt(m_con.GetProbeResult().m_epModem.write);

	const unsigned char *b = data.GetData(), *e = data.GetData() + data.GetSize();

	if( !m_ppp_mode ) {
		if( *b == 0x7e ) {
			m_ppp_mode = true;
			// fall through
		}
		else {
			m_dev.BulkWrite(m_con.GetProbeResult().m_epModem.write,
				data, timeout);
			return;
		}
	}

	size_t needed = data.GetSize() / 2 * 3 + 4;	// worst case
	unsigned char *buf = m_writeBuf.GetBuffer(needed);
	unsigned char *put = buf;

	while( b != e ) {
		// if last character was 0x7e, then next one must be, or else we
		// insert it ourselves
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
	m_dev.BulkWrite(m_con.GetProbeResult().m_epModem.write, m_writeBuf, timeout);
}

}} // namespace Barry::Mode

