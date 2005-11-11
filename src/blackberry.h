///
/// \file	blackberry.h
///		High level BlackBerry API class
///

#ifndef __SYNCBERRY_BLACKBERRY_H__
#define __SYNCBERRY_BLACKBERRY_H__

#include "usbwrap.h"
#include "probe.h"
#include "socket.h"

namespace Syncberry {

class Blackberry
{
	Usb::Device m_dev;
	Usb::Interface m_iface;
	uint32_t m_pin;

	Socket m_socket;

public:
	Blackberry(const ProbeResult &device);
	~Blackberry();

	void Test();
};

} // namespace Syncberry

#endif

