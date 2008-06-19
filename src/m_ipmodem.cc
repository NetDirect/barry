///
/// \file	m_ipmodem.cc
///		Mode class for GPRS modem mode (using endpoints on
///		modern devices)
///

/*
    Copyright (C) 2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "m_ipmodem.h"
#include "controller.h"
#include "data.h"
#include "debug.h"
#include <sstream>
#include <string.h>
#include <openssl/sha.h>

namespace Barry { namespace Mode {

const char special_flag[] = { 0x78, 0x56, 0x34, 0x12 };	// 0x12345678
const char start[] = { 0x01, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };
const char pw_start[] = { 0x01, 0, 0, 0, 1, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };

// Global variable ok?
unsigned char session_key[] = { 0x00, 0, 0, 0, 0, 0, 0, 0 };

//////////////////////////////////////////////////////////////////////////////
// Mode::IpModem class

IpModem::IpModem(Controller &con,
		DeviceDataCallback callback,
		void *callback_context)
	: m_con(con)
	, m_dev(con.m_dev)
	, m_continue_reading(false)
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

bool IpModem::SendPassword(const char *password)
{
	if( strlen(password) == 0  ) {
		throw BadPassword("No password specified.", 0, false);
	}
	int timeout = -1;
	int read_ep  = m_con.GetProbeResult().m_epModem.read;
	int write_ep = m_con.GetProbeResult().m_epModem.write;
	Data data;

	m_dev.BulkWrite(write_ep, pw_start, sizeof(pw_start), timeout);
	m_dev.BulkRead(read_ep, data, 5000);

	// Need to add checks for other packet types.
	// check for 02 00 00 00 SS SS SS SS RR 00 00 00 0a 00 00 00 PP PP PP PP PP 00 00 00 78 56 34 12
        if( data.GetSize() > 4 && data.GetData()[0] == 0x02  &&
	    memcmp(data.GetData() + data.GetSize() - 4, special_flag, sizeof(special_flag))== 0 ) {
		// Got a password request packet
		ddout("IPModem password request packet:\n" << data);

		// Check how many retries are left
		if( data.GetData()[8] < 6 ) {
			throw BadPassword("Fewer than 6 password tries "
				"remaining in device. Refusing to proceed, "
				"to avoid device zapping itself.  Use a "
				"Windows client, or re-cradle the device.",
				data.GetData()[8],
				true);
		}

		// Build the password hash
		unsigned char pwdigest[SHA_DIGEST_LENGTH];
		unsigned char prefixedhash[SHA_DIGEST_LENGTH + 4];
		unsigned char pw_response[SHA_DIGEST_LENGTH + 8];
		unsigned char seed[4];

		// first, hash the password by itself
		SHA1((unsigned char *) password, strlen(password), pwdigest);

		// prefix the resulting hash with the provided seed
		memcpy(&seed[0], data.GetData() + 4, sizeof(uint32_t));	// FIXME for endian support?
		memcpy(&prefixedhash[0], &seed, sizeof(uint32_t));
		memcpy(&prefixedhash[4], pwdigest, SHA_DIGEST_LENGTH);

		// hash again
		SHA1((unsigned char *) prefixedhash, SHA_DIGEST_LENGTH + 4, pwdigest);

		// Build the response packet
		const char pw_rsphdr[]  = { 0x03, 0x00, 0x00, 0x00 };
		memcpy(&pw_response[0], pw_rsphdr, sizeof(pw_rsphdr));
		memcpy(&pw_response[4], pwdigest, SHA_DIGEST_LENGTH);
		memcpy(&pw_response[24], special_flag, sizeof(special_flag));

		// Send the password response packet
		m_dev.BulkWrite(write_ep, pw_response, sizeof(pw_response), timeout);
		m_dev.BulkRead(read_ep, data, 5000);

		// check response 04 00 00 00 .......
		// the seed is incremented, retries are reset to 10 when the password is accepted.
                if( data.GetData()[0] == 0x04  && data.GetData()[8] == 0x0a ) {
			ddout("IPModem password accepted.\n");

			// send "start"? packet
			m_dev.BulkWrite(write_ep, pw_start, sizeof(pw_start), timeout);

			// send packet with the last 8 bytes of the password hash (session_key?)
			//unsigned char pw_response[SHA_DIGEST_LENGTH + 8];
			unsigned char pw_rsphdr[] = { 0x00, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0xc2, 1, 0 };
			//unsigned char session_key[8];
			memcpy(&session_key[0], pwdigest + 12, 8);
			memcpy(&pw_response[0], pw_rsphdr, sizeof(pw_rsphdr));
			memcpy(&pw_response[16], pwdigest + 12, 8);
			//memcpy(&pw_response[16], session_key, 8);
			memcpy(&pw_response[24], special_flag, sizeof(special_flag));
			m_dev.BulkWrite(write_ep, pw_response, sizeof(pw_response), timeout);

			// blank password hashes as we don't need these anymore
			memset(pwdigest, 0, sizeof(pwdigest));
			memset(prefixedhash, 0, sizeof(prefixedhash));
			// The modem should be ready to accept AT commands
			return true;
		}
		else {  //FIXME need to clean up better....
			// Wrong packet or invalid password
			ddout("IPModem invalid password.\n" << data);
			throw Barry::Error("IpModem:: Error invalid password.");
		}
	}
	return false;
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

void IpModem::Open(const char *password)
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

	if( strlen(password) != 0 ) {
		if( !SendPassword(password) ) {
			throw Barry::Error("IpModem:: Error sending password.");
		}
	}
	else {
		Data block(start, sizeof(start));
		Write(block);
	}

	// spawn read thread
	m_continue_reading = true;
	int ret = pthread_create(&m_modem_read_thread, NULL, &IpModem::DataReadThread, this);
	if( ret ) {
		m_continue_reading = false;
		throw Barry::ErrnoError("IpModem:: Error creating USB read thread.", ret);
	}
}

void IpModem::Write(const Data &data, int timeout)
{
	if( data.GetSize() == 0 )
		return;	// nothing to do

//	m_dev.ClearHalt(m_con.GetProbeResult().m_epModem.write);

	m_dev.BulkWrite(m_con.GetProbeResult().m_epModem.write,
		m_filter.Write(data), timeout);
}

void IpModem::Close()
{
	// This is the terminate connection sequence
	// that resets the modem so we can re-connect
	// without unpluging the USB cable or reseting
	// the whole device.
	// This works on a BB 8703e a with password. other BB's??
	unsigned char end[28];
	int timeout = -1;
	int read_ep  = m_con.GetProbeResult().m_epModem.read;
	int write_ep = m_con.GetProbeResult().m_epModem.write;
	Data data;

	ddout("IpModem:: Closing connection.");

	// Wait a bit for the modem to stop sending data
	// before stopping the read thread.
	sleep(4);

	// stop the read thread
	if( m_continue_reading ) {
		m_continue_reading = false;
		pthread_join(m_modem_read_thread, NULL);
	}

	//0 0 0 0 b0 0 0 0 0 0 0 0 0 c2 1 0 + session_key + special_flag
	memset(end, 0, sizeof(end));
	end[4]  = 0xb0;
	end[13] = 0xc2;
	end[14] = 0x01;
	memcpy(&end[16], session_key,  sizeof(session_key));
	memcpy(&end[24], special_flag, sizeof(special_flag));
	m_dev.BulkWrite(write_ep, end, sizeof(end), timeout);

	//FIXME move try-catch-read to a drainRead function
	//	since we're just throwing the packets away
	try {
		m_dev.BulkRead(read_ep, data, 5000);
		ddout("IPModem read packet:\n" << data);
	}
	catch( Usb::Timeout &to ) {
		// do nothing on timeouts
		ddout("Timeout in DataReadThread!");
	}

	//0 0 0 0 20 0 0 0 3 0 0 0 0 c2 1 0 + session_key + special_flag
	memset(end, 0, sizeof(end));
	end[4]  = 0x20;
	end[8]  = 0x03;
	end[13] = 0xc2;
	end[14] = 0x01;
	memcpy(&end[16], session_key,  sizeof(session_key));
	memcpy(&end[24], special_flag, sizeof(special_flag));
	m_dev.BulkWrite(write_ep, end, sizeof(end), timeout);
	try {
		m_dev.BulkRead(read_ep, data, 5000);
		ddout("IPModem read packet:\n" << data);
	}
	catch( Usb::Timeout &to ) {
		// do nothing on timeouts
		ddout("Timeout in DataReadThread!");
	}

	//0 0 0 0 30 0 0 0 0 0 0 0 0 c2 1 0 + session_key + special_flag
	memset(end, 0, sizeof(end));
	end[4]  = 0x30;
	end[13] = 0xc2;
	end[14] = 0x01;
	memcpy(&end[16], session_key,  sizeof(session_key));
	memcpy(&end[24], special_flag, sizeof(special_flag));
	m_dev.BulkWrite(write_ep, end, sizeof(end), timeout);
	try {
		m_dev.BulkRead(read_ep, data, 5000);
		ddout("IPModem read packet:\n" << data);
	}
	catch( Usb::Timeout &to ) {
		// do nothing on timeouts
		ddout("Timeout in DataReadThread!");
	}

	const char stop[] = { 0x01, 0, 0, 0, 0, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };
	m_dev.BulkWrite(write_ep, stop, sizeof(stop), timeout);
	try {
		m_dev.BulkRead(read_ep, data, 5000);
		ddout("IPModem read packet:\n" << data);
	}
	catch( Usb::Timeout &to ) {
		// do nothing on timeouts
		ddout("Timeout in DataReadThread!");
	}
}


}} // namespace Barry::Mode

