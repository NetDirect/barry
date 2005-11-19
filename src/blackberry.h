///
/// \file	blackberry.h
///		High level BlackBerry API class
///

#ifndef __BARRY_BLACKBERRY_H__
#define __BARRY_BLACKBERRY_H__

#include "usbwrap.h"
#include "probe.h"
#include "socket.h"
#include "record.h"

namespace Barry {

class Blackberry
{
public:
	enum ModeType { Unspecified, Bypass, Desktop, JavaLoader };
	enum CommandType { Unknown, DatabaseAccess };

private:
	Usb::Device m_dev;
	Usb::Interface m_iface;
	uint32_t m_pin;

	Socket m_socket;

	CommandTable m_commandTable;

	DatabaseDatabase m_dbdb;

	ModeType m_mode;

protected:
	void SelectMode(ModeType mode, uint16_t &socket, uint8_t &flag);
	void OpenMode(ModeType mode);
	unsigned int GetCommand(CommandType ct);

public:
	Blackberry(const ProbeResult &device);
	~Blackberry();

	void Test();

	void GetCommandTable();
	void GetDBDB();
	void GetAddressBook();
};

} // namespace Barry

#endif

