///
/// \file	m_serial.cc
///		Mode class for serial / GPRS modem mode
///

#include "m_serial.h"
#include "controller.h"
#include "protostructs.h"
#include "endian.h"
#include <stdexcept>

namespace Barry { namespace Mode {

//////////////////////////////////////////////////////////////////////////////
// Mode::Serial class

Serial::Serial(	Controller &con,
		DeviceDataCallback callback,
		void *callback_context)
	: m_con(con)
	, m_ModeSocket(0)
	, m_CtrlSocket(0)
	, m_callback(callback)
	, m_callback_context(callback_context)
{
	if( !m_con.HasQueue() )
		throw std::logic_error("A SocketRoutingQueue is required in the Controller class when using Mode::Serial.");
}

Serial::~Serial()
{
}


//////////////////////////////////////////////////////////////////////////////
// protected API / static functions

void Serial::DataCallback(void *context, Data *data)
{
	Serial *ser = (Serial*) context;

	if( data->GetSize() <= 4 )
		return;	// nothing to do

	// call callback if available
	if( ser->m_callback ) {
		(*ser->m_callback)(ser->m_callback_context,
			data->GetData() + 4,
			data->GetSize() - 4);
	}
//	else {
//		// append data to readCache
//		FIXME;
//	}
}

//////////////////////////////////////////////////////////////////////////////
// public API

void Serial::Open(const char *password)
{
	if( m_ModeSocket ) {
		m_data->Close();
		m_data.reset();
		m_ModeSocket = 0;
	}

	if( m_CtrlSocket ) {
		m_ctrl->Close();
		m_ctrl.reset();
		m_CtrlSocket = 0;
	}

	m_ModeSocket = m_con.SelectMode(Controller::UsbSerData);
	m_CtrlSocket = m_con.SelectMode(Controller::UsbSerCtrl);
	RetryPassword(password);
}

// FIXME - if this behaviour is truly common between modes, create
// a common base class for this.
void Serial::RetryPassword(const char *password)
{
	if( m_data.get() || m_ctrl.get() )
		throw std::logic_error("Socket already open in Serial::RetryPassword");

	m_data = m_con.m_zero.Open(m_ModeSocket, password);
	m_ctrl = m_con.m_zero.Open(m_CtrlSocket, password);

	// register callback for incoming data, for speed
	m_data->RegisterInterest(DataCallback, this);
}

/*
// can be called from separate thread
void Serial::SerialRead(Data &data, int timeout)
{
	m_socket.Receive(data, timeout);
}
*/

// based on Rick Scott's XmBlackBerry's serdata.c
void Serial::Write(const Data &data)
{
	if( data.GetSize() <= 0 )
		return;	// nothing to do

	// make room in write cache buffer for the 4 byte header
	int size = data.GetSize() + 4;
	unsigned char *buf = m_writeCache.GetBuffer(size);
	MAKE_PACKETPTR_BUF(spack, buf);

	// copy data over to cache packet
	memcpy(&buf[4], data.GetData(), data.GetSize());

	// setup header (only size needed, as socket will be set by socket class)
	spack->size = htobs(size);

	// release and send
	m_writeCache.ReleaseBuffer(size);
	m_data->Send(m_writeCache);

///////////////////////////////////////////////////////////////////////
//	unsigned char buf[0x400];
//	int num_read;
//	int i;
//
//	//
//	// This is pretty ugly, but I have to put the HDLC flags into
//	// the packets. RIM seems to need flags around every frame, and
//	// a flag _cannot_ be an end and a start flag.
//	//
//	for (i = 0; i < num_read; i++) {
//		BufferAdd(&serdata->data, &buf[i], 1);
//		if (BufferData(&serdata->data)[0] == 0x7e && buf[i] == 0x7e) {
//			if (BufferLen(&serdata->data) > 1 &&
//				BufferData(&serdata->data)[0] == 0x7e && 
//				BufferData(&serdata->data)[1] == 0x7e)
//			{
//				BufferPullHead(&serdata->data, 1);
//			}
//			else
//			{
//			}
//			if (BufferLen(&serdata->data) > 2)
//			{
//				if ((BufferLen(&serdata->data) + 4) % 16 == 0)
//				{
//					BufferAdd(&serdata->data, (unsigned char *)"\0", 1);
//				}
//				send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
//				BufferEmpty(&serdata->data);
//				BufferAdd(&serdata->data, (unsigned char *)"\176", 1);
//			}
//			if (BufferLen(&serdata->data) == 2)
//			{
//				BufferPullTail(&serdata->data, 1);
//			}
//			else
//			{
//			}
//		}
//		else
//		{
//		}
//	}
//	if (BufferData(&serdata->data)[0] == 0x7e &&
//	   memcmp(&BufferData(&serdata->data)[1], "AT", 2) == 0)
//	{
//		BufferPullHead(&serdata->data, 1);
//	}
//	if (BufferData(&serdata->data)[0] != 0x7e)
//	{
//		debug(9, "%s:%s(%d) - %i\n",
//			__FILE__, __FUNCTION__, __LINE__,
//			BufferLen(&serdata->data));
//		send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
//		BufferEmpty(&serdata->data);
//	}
}

}} // namespace Barry::Mode

