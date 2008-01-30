///
/// \file	m_serial.cc
///		Mode class for serial / GPRS modem mode
///

#include "m_serial.h"

namespace Barry { namespace Mode {

//////////////////////////////////////////////////////////////////////////////
// UsbSerData mode - modem specific

#if 0

for the Open() call:
	case UsbSerData:
		// open the SerCtrl mode as well
		uint16_t modeSocket = SelectMode(UsbSerCtrl);
		m_serCtrlSocket.Open(modeSocket);
		break;


// can be called from separate thread
void Controller::SerialRead(Data &data, int timeout)
{
	if( m_mode != UsbSerData )
		throw std::logic_error("Wrong mode in SerialRead");

	m_socket.Receive(data, timeout);
}

// based on Rick Scott's XmBlackBerry's serdata.c
void Controller::SerialWrite(const Data &data)
{
	if( m_mode != UsbSerData )
		throw std::logic_error("Wrong mode in SerialWrite");

	if( data.GetSize() <= 0 )
		return;	// nothing to do

	int size = data.GetSize() + 4;
	unsigned char *buf = m_writeCache.GetBuffer(size);
	MAKE_PACKETPTR_BUF(spack, buf);

	// copy data over to cache packet
	memcpy(&buf[4], data.GetData(), data.GetSize());

	// setup header
	spack->socket = htobs(m_socket.GetSocket());
	spack->size = htobs(size);

	// release and send
	m_writeCache.ReleaseBuffer(size);
	m_socket.Send(m_writeCache);

/*
	unsigned char buf[0x400];
	int num_read;
	int i;

	//
	// This is pretty ugly, but I have to put the HDLC flags into
	// the packets. RIM seems to need flags around every frame, and
	// a flag _cannot_ be an end and a start flag.
	//
	for (i = 0; i < num_read; i++) {
		BufferAdd(&serdata->data, &buf[i], 1);
		if (BufferData(&serdata->data)[0] == 0x7e && buf[i] == 0x7e) {
			if (BufferLen(&serdata->data) > 1 &&
				BufferData(&serdata->data)[0] == 0x7e && 
				BufferData(&serdata->data)[1] == 0x7e)
			{
				BufferPullHead(&serdata->data, 1);
			}
			else
			{
			}
			if (BufferLen(&serdata->data) > 2)
			{
				if ((BufferLen(&serdata->data) + 4) % 16 == 0)
				{
					BufferAdd(&serdata->data, (unsigned char *)"\0", 1);
				}
				send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
				BufferEmpty(&serdata->data);
				BufferAdd(&serdata->data, (unsigned char *)"\176", 1);
			}
			if (BufferLen(&serdata->data) == 2)
			{
				BufferPullTail(&serdata->data, 1);
			}
			else
			{
			}
		}
		else
		{
		}
	}
	if (BufferData(&serdata->data)[0] == 0x7e &&
	   memcmp(&BufferData(&serdata->data)[1], "AT", 2) == 0)
	{
		BufferPullHead(&serdata->data, 1);
	}
	if (BufferData(&serdata->data)[0] != 0x7e)
	{
		debug(9, "%s:%s(%d) - %i\n",
			__FILE__, __FUNCTION__, __LINE__,
			BufferLen(&serdata->data));
		send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
		BufferEmpty(&serdata->data);
	}
*/
}
#endif

}} // namespace Barry::Mode

