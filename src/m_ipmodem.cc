///
/// \file	m_ipmodem.cc
///		Mode class for GPRS modem mode (using endpoints on
///		modern devices)
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "sha1.h"

namespace Barry { namespace Mode {

const char special_flag[] = { 0x78, 0x56, 0x34, 0x12 };	// 0x12345678
const char start[] 	  = { 0x01, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };
const char pw_start[] 	  = { 0x01, 0, 0, 0, 1, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };
const char stop[] 	  = { 0x01, 0, 0, 0, 0, 0, 0, 0, 0x78, 0x56, 0x34, 0x12 };

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
	memset(m_session_key, 0, sizeof(m_session_key));
}

IpModem::~IpModem()
{
	try {
		Close();
	} catch( std::exception &e ) {
		dout("Exception caught in IpModem destructor, ignoring: "
			<< e.what());
	}
}

bool IpModem::SendPassword( const char *password, uint32_t seed )
{
	if( !password || strlen(password) == 0  ) {
		throw BadPassword("Logic error: No password provided in SendPassword.", 0, false);
	}

	int read_ep  = m_con.GetProbeResult().m_epModem.read;
	int write_ep = m_con.GetProbeResult().m_epModem.write;
	unsigned char pwdigest[SHA_DIGEST_LENGTH];
	unsigned char prefixedhash[SHA_DIGEST_LENGTH + 4];
	unsigned char pw_response[SHA_DIGEST_LENGTH + 8];
	uint32_t new_seed;
	Data data;

	if( !password || strlen(password) == 0  ) {
		throw BadPassword("No password provided.", 0, false);
	}

	// Build the password hash
	// first, hash the password by itself
	SHA1((unsigned char *) password, strlen(password), pwdigest);

	// prefix the resulting hash with the provided seed
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
	m_dev.BulkWrite(write_ep, pw_response, sizeof(pw_response));
	m_dev.BulkRead(read_ep, data);
	ddout("IPModem: Read password response.\n" << data);

	// Added for the BB Storm 9000's second password request
	if( data.GetSize() >= 16 && data.GetData()[0] == 0x00 ) {
		try {
			m_dev.BulkRead(read_ep, data, 500);
			ddout("IPModem: Null Response Packet:\n" << data);
		}
		catch( Usb::Timeout &to ) {
			// do nothing on timeouts
			ddout("IPModem: Null Response Timeout");
		}
	}

	//
	// check response 04 00 00 00 .......
	// On the 8703e the seed is incremented, retries are reset to 10
	// when the password is accepted.
	//
	// If data.GetData() + 4 is = to the orginal seed +1 or 00 00 00 00
	// then the password was acceppted.
	//
	// When data.GetData() + 4 is not 00 00 00 00 then data.GetData()[8]
	// contains the number of password retrys left.
	//
	if( data.GetSize() >= 9 && data.GetData()[0] == 0x04 ) {
		memcpy(&new_seed, data.GetData() + 4, sizeof(uint32_t));
		seed++;
		if( seed == new_seed || new_seed == 0 ) {
			ddout("IPModem: Password accepted.\n");

#if SHA_DIGEST_LENGTH < SB_IPMODEM_SESSION_KEY_LENGTH
#error Session key field must be smaller than SHA digest
#endif
			// Create session key - last 8 bytes of the password hash
			memcpy(&m_session_key[0],
				pwdigest + SHA_DIGEST_LENGTH - sizeof(m_session_key),
				sizeof(m_session_key));

			// blank password hashes as we don't need these anymore
			memset(pwdigest, 0, sizeof(pwdigest));
			memset(prefixedhash, 0, sizeof(prefixedhash));
			return true;
		}
		else {
			ddout("IPModem: Invalid password.\n" << data);
			throw BadPassword("Password rejected by device.", data.GetData()[8], false);
		}
	}
	// Unknown packet
	ddout("IPModem: Error unknown packet.\n" << data);
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
				ddout("IPModem: Special packet:\n" << data);
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
			ddout("IPModem: Timeout in DataReadThread!");
		}
		catch( std::exception &e ) {
			eout("Exception in IpModem::DataReadThread: " << e.what());
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// public API

void IpModem::Open(const char *password)
{
	int read_ep  = m_con.GetProbeResult().m_epModem.read;
	int write_ep = m_con.GetProbeResult().m_epModem.write;
	unsigned char response[28];
	uint32_t seed;
	Data data;

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

	// Send stop command
	ddout("IPModem: Sending Stop Response:\n");
	m_dev.BulkWrite(write_ep, stop, sizeof(stop));
	try {
		m_dev.BulkRead(read_ep, data, 500);
		ddout("IPModem: Stop Response Packet:\n" << data);
	}
	catch( Usb::Timeout &to ) {
		// do nothing on timeouts
		ddout("IPModem: Stop Response Timeout");
	}

	// Send start commands to figure out if the device needs a password.
	ddout("IPModem: Sending Start Response:\n");
	m_dev.BulkWrite(write_ep, pw_start, sizeof(pw_start));
	m_dev.BulkRead(read_ep, data, 5000);
	ddout("IPModem: Start Response Packet:\n" << data);

	// check for 02 00 00 00 SS SS SS SS RR 00 00 00 0a 00 00 00 PP PP PP PP PP 00 00 00 78 56 34 12
        if( data.GetSize() >= 9 && data.GetData()[0] == 0x02  &&
	    memcmp(data.GetData() + data.GetSize() - 4, special_flag, sizeof(special_flag))== 0 ) {
		// Got a password request packet
		ddout("IPModem: Password request packet:\n" << data);

		// Check how many retries are left
		if( data.GetData()[8] < BARRY_MIN_PASSWORD_TRIES ) {
			throw BadPassword("Fewer than " BARRY_MIN_PASSWORD_TRIES_ASC " password tries remaining in device. Refusing to proceed, to avoid device zapping itself.  Use a Windows client, or re-cradle the device.",
				data.GetData()[8],
				true);
		}
		memcpy(&seed, data.GetData() + 4, sizeof(seed));
		// Send password
		if( !SendPassword(password, seed) ) {
			throw Barry::Error("IpModem: Error sending password.");
		}

		// Re-send "start" packet
		ddout("IPModem: Re-sending Start Response:\n");
		m_dev.BulkWrite(write_ep, pw_start, sizeof(pw_start));
		m_dev.BulkRead(read_ep, data);
		ddout("IPModem: Start Response Packet:\n" << data);
	}

	// send packet with the session_key
	unsigned char response_header[] = { 0x00, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0xc2, 1, 0 };
	memcpy(&response[0], response_header, sizeof(response_header));
	memcpy(&response[16], m_session_key,  sizeof(m_session_key));
	memcpy(&response[24], special_flag, sizeof(special_flag));
	ddout("IPModem: Sending Session key:\n");
	m_dev.BulkWrite(write_ep, response, sizeof(response));
	if( data.GetSize() >= 16 ) {
		switch(data.GetData()[0])
		{
		case 0x00:	// Null packet
			break;

		case 0x02:	// password seed received
			memcpy(&seed, data.GetData() + 4, sizeof(uint32_t));
			if( !SendPassword( password, seed ) ) {
				throw Barry::Error("IpModem: Error sending password.");
			}
			break;
		case 0x04:	// command accepted
			break;

		default: 	// ???
			ddout("IPModem: Unknown response.\n");
			break;
		}
	}

	// see if the modem will respond to commands
	const char modem_command[] = { "AT\r" };
	m_dev.BulkWrite(write_ep, modem_command, strlen(modem_command));
	m_dev.BulkRead(read_ep, data);
	ddout("IPModem: Test command response.\n" << data);
	if( data.GetSize() >= 1 ) {
		switch(data.GetData()[0])
		{
		case 0x00:	// Null packet
			try {
				m_dev.BulkRead(read_ep, data, 5000);
				ddout("IPModem: AT Response Packet:\n" << data);
			}
			catch( Usb::Timeout &to ) {
				// do nothing on timeouts
				ddout("IPModem: AT Response Timeout");
			}
			break;

		case 0x02:	// password seed received
			if( !password || strlen(password) == 0 ) {
				throw BadPassword("This device requested a password.",
					data.GetSize() >= 9 ? data.GetData()[8] : 0, false);
			}
			else {	// added for the Storm 9000
				memcpy(&seed, data.GetData() + 4, sizeof(seed));
				if( !SendPassword( password, seed ) ) {
					throw Barry::Error("IpModem: Error sending password.");
				}
			}
			break;
		case 0x04:	// command accepted
			break;

		case 0x07:	// device is password protected?
			throw BadPassword("This device requires a password.", 0, false);

		default: 	// ???
			ddout("IPModem: Unknown AT command response.\n");
			break;
		}
	}
	ddout("IPModem: Modem Ready.\n");

	// spawn read thread
	m_continue_reading = true;
	int ret = pthread_create(&m_modem_read_thread, NULL, &IpModem::DataReadThread, this);
	if( ret ) {
		m_continue_reading = false;
		throw Barry::ErrnoError("IpModem: Error creating USB read thread.", ret);
	}
}

void IpModem::Write(const Data &data, int timeout)
{
	if( data.GetSize() == 0 )
		return;	// nothing to do

	// according to Rick Scott the m_filter is not needed with the ip modem
	// but with the 8320 with Rogers, it doesn't seem to connect without it
	// If this is a performance problem, perhaps make this a runtime
	// option.
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
	int read_ep  = m_con.GetProbeResult().m_epModem.read;
	int write_ep = m_con.GetProbeResult().m_epModem.write;
	Data data;

	//0 0 0 0 b0 0 0 0 0 0 0 0 0 c2 1 0 + session_key + special_flag
	ddout("IpModem: Closing connection.");
	memset(end, 0, sizeof(end));
	end[4]  = 0xb0;
	end[13] = 0xc2;
	end[14] = 0x01;
	memcpy(&end[16], m_session_key,  sizeof(m_session_key));
	memcpy(&end[24], special_flag, sizeof(special_flag));
	m_dev.BulkWrite(write_ep, end, sizeof(end));

	//0 0 0 0 20 0 0 0 3 0 0 0 0 c2 1 0 + session_key + special_flag
	memset(end, 0, sizeof(end));
	end[4]  = 0x20;
	end[8]  = 0x03;
	end[13] = 0xc2;
	end[14] = 0x01;
	memcpy(&end[16], m_session_key,  sizeof(m_session_key));
	memcpy(&end[24], special_flag, sizeof(special_flag));
	m_dev.BulkWrite(write_ep, end, sizeof(end));

	//0 0 0 0 30 0 0 0 0 0 0 0 0 c2 1 0 + session_key + special_flag
	// The session_key is set to 0x0's when there is no password.
	memset(end, 0, sizeof(end));
	end[4]  = 0x30;
	end[13] = 0xc2;
	end[14] = 0x01;
	memcpy(&end[16], m_session_key,  sizeof(m_session_key));
	memcpy(&end[24], special_flag, sizeof(special_flag));
	m_dev.BulkWrite(write_ep, end, sizeof(end));
	m_dev.BulkWrite(write_ep, stop, sizeof(stop));
	try {
		m_dev.BulkRead(read_ep, data, 5000);
		ddout("IPModem: Close read packet:\n" << data);
	}
	catch( Usb::Timeout &to ) {
		// do nothing on timeouts
		ddout("IPModem: Close Read Timeout");
	}
	// stop the read thread
	if( m_continue_reading ) {
		m_continue_reading = false;
		pthread_join(m_modem_read_thread, NULL);
	}
	ddout("IPmodem: Closed!");

}

}} // namespace Barry::Mode

