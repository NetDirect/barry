///
/// \file	usbwrap.h
///		USB API wrapper
///

/*
    Copyright (C) 2005-2012, Chris Frey
    Portions Copyright (C) 2011, RealVNC Ltd.

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


#ifndef __SB_USBWRAP_H__
#define __SB_USBWRAP_H__

#include "dll.h"

#include <memory>
#include <tr1/memory>
#include <vector>
#include <map>
#include "error.h"

#define USBWRAP_DEFAULT_TIMEOUT	30000

// Matches any product ID when calling DeviceList::MatchDevices
#define PRODUCT_ANY             0x10000
// Indicates an unknown product ID
#define PRODUCT_UNKNOWN         0x20000

namespace Barry { class Data; }

/// Namespace for the libusb-related wrapper classes.  This namespace
/// may change in the future.
namespace Usb {

/// \addtogroup exceptions
/// @{

/// Thrown on low level USB errors.
class BXEXPORT Error : public Barry::Error
{
	int m_libusb_errcode;

public:
	Error(const std::string &str);
	Error(int libusb_errcode, const std::string &str);

	// can return 0 in some case, if unknown error code
	int libusb_errcode() const { return m_libusb_errcode; }

	// returns a translated system error code when using libusb 1.0
	// returns 0 if unknown or unable to translate
	int system_errcode() const;
};

class BXEXPORT Timeout : public Error
{
public:
	Timeout(const std::string &str) : Error(str) {}
	Timeout(int errcode, const std::string &str)
		: Error(errcode, str) {}
};

/// @}

// Private struct for holding library specific
// a unique identifier to a connected device.
class DeviceIDImpl;

class BXEXPORT DeviceID
{
public:
	std::tr1::shared_ptr<DeviceIDImpl> m_impl;
public:
	// Takes ownership of impl
	DeviceID(DeviceIDImpl* impl = NULL);
	~DeviceID();
	const char* GetBusName() const;
	uint16_t GetNumber() const;
	const char* GetFilename() const;
	uint16_t GetIdProduct() const;

	// Utility function: returns a string that uniquely identifies
	// the bus and device, regardless of which libusb you're using
	std::string GetUsbName() const;
};

// Private struct for holding a library specific
// device handle
struct DeviceHandle;

// Static functions for setting up USB
// The interface that usbwrap.cc uses
// to interact with the USB library
class BXEXPORT LibraryInterface
{
public:
	static std::string GetLastErrorString(int libusb_errcode);

	/// Returns 0 if unable to translate libusb error code.
	/// Note that this function assumes you already know that libusb_errcode
	/// contains an actual error code, and so returns 0 (success)
	/// for an unknown error.  This means that "success" means error
	/// if you use this function correctly, but if you pass in a success
	/// code (>= 0) it will always return 0 as well.
	static int TranslateErrcode(int libusb_errcode);

	/// Returns true on success... pass in a pointer to int
	/// if the low level error code is important to you.
	static bool Init(int *libusb_errno = 0);
	static void Uninit();
	static void SetDataDump(bool data_dump_mode);
};

// Forward declaration of descriptor types.
class BXEXPORT DeviceDescriptor;
class BXEXPORT ConfigDescriptor;
class BXEXPORT InterfaceDescriptor;

// Private struct for holding library specific
// information about endpoint descriptors
struct EndpointDescriptorImpl;

// Encapsulates an endpoint descriptor
class BXEXPORT EndpointDescriptor
{
public:
	enum EpType
	{
		ControlType = 0,
		IsochronousType = 1,
		BulkType = 2,
		InterruptType = 3,
		InvalidType = 0xff
	};
private:
	const std::auto_ptr<EndpointDescriptorImpl> m_impl;
	bool m_read;
	uint8_t m_addr;
	EpType m_type;
private:
	EndpointDescriptor(const EndpointDescriptor& rhs); // Prevent copying
public:
	EndpointDescriptor(InterfaceDescriptor& dev, int endpoint);
	~EndpointDescriptor();
	bool IsRead() const;
	uint8_t GetAddress() const;
	EpType GetType() const;
};

// Private struct for holding library specific
// information about interface descriptors
struct InterfaceDescriptorImpl;

// Encapsulates an interface descriptor
//
// The inherited vector methods look up endpoints
class BXEXPORT InterfaceDescriptor : public std::vector<EndpointDescriptor*>
{
	friend class EndpointDescriptor;
public:
	typedef std::vector<EndpointDescriptor*> base_type;
private:
	const std::auto_ptr<InterfaceDescriptorImpl> m_impl;
private:
	InterfaceDescriptor(const InterfaceDescriptor& rhs); // Prevent copying
public:
	InterfaceDescriptor(ConfigDescriptor& cfgdesc,
			    int iface, int altsetting);
	~InterfaceDescriptor();
	uint8_t GetClass() const;
	uint8_t GetNumber() const;
	uint8_t GetAltSetting() const;
};

// Private struct for holding library specific
// information about config descriptors

struct ConfigDescriptorImpl;

// Encapsulates a configuration descriptor
//
// The inherited map methods look up interface descriptors
class BXEXPORT ConfigDescriptor : public std::map<int, InterfaceDescriptor*>
{
	friend class InterfaceDescriptor;
public:
	typedef std::map<int, InterfaceDescriptor*> base_type;
private:
	const std::auto_ptr<ConfigDescriptorImpl> m_impl;
private:
	ConfigDescriptor(const ConfigDescriptor& rhs); // Prevent copying
public:
	ConfigDescriptor(DeviceDescriptor& dev, int cfgnumber);
	~ConfigDescriptor();
	uint8_t GetNumber() const;
};

// Private struct for holding library specific
// information about a device descriptor
struct DeviceDescriptorImpl;

// Encapsulates a device descriptor
//
// The inherited map methods look up config descriptors
class BXEXPORT DeviceDescriptor : public std::map<int, ConfigDescriptor*>
{
	friend class ConfigDescriptor;
public:
	typedef std::map<int, ConfigDescriptor*> base_type;
private:
	const std::auto_ptr<DeviceDescriptorImpl> m_impl;
private:
	DeviceDescriptor(const DeviceDescriptor& rhs); // Prevent copying
public:
	DeviceDescriptor(DeviceID& devid);
	~DeviceDescriptor();
};

// Private struct for holding library specific
// information for devices.
struct DeviceListImpl;

class BXEXPORT DeviceList
{
private:
	// Private implementation structure
	const std::auto_ptr<DeviceListImpl> m_impl;
private:
	DeviceList(const DeviceList& rhs); // Prevent copying
public:
	DeviceList();
	~DeviceList();

	std::vector<DeviceID> MatchDevices(int vendor, int product,
					   const char *busname, const char *devname);

};

struct PrivateDeviceData;

class BXEXPORT Device
{
private:
	Usb::DeviceID m_id;
	std::auto_ptr<Usb::DeviceHandle> m_handle;

	int m_timeout;
	int m_lasterror;
private:
	Device(const Device& rhs); // Prevent copying
public:
	Device(const Usb::DeviceID& id, int timeout = USBWRAP_DEFAULT_TIMEOUT);
	~Device();

	/////////////////////////////
	// Data access

	const Usb::DeviceID& GetID() const { return m_id; }
	const Usb::DeviceHandle* GetHandle() const { return &*m_handle; }
	int GetLastError() const { return m_lasterror; } //< not thread safe...
		//< use the error code stored in the exceptions to track
		//< errors in threaded usage
	void SetLastError(int err) { m_lasterror = err; }
	int GetDefaultTimeout() const { return m_timeout; }

	/////////////////////////////
	// Device information

	int GetPowerLevel();
	int FindInterface(int ifaceClass);

	/////////////////////////////
	// Device manipulation

	bool SetConfiguration(unsigned char cfg);
	bool ClearHalt(int ep);
	bool Reset();
	bool IsAttachKernelDriver(int iface);
	bool DetachKernelDriver(int iface);

	/////////////////////////////
	// IO functions

	bool BulkRead(int ep, Barry::Data &data, int timeout = -1);
	bool BulkWrite(int ep, const Barry::Data &data, int timeout = -1);
	bool BulkWrite(int ep, const void *data, size_t size, int timeout = -1);
	bool InterruptRead(int ep, Barry::Data &data, int timeout = -1);
	bool InterruptWrite(int ep, const Barry::Data &data, int timeout = -1);

	void BulkDrain(int ep, int timeout = 100);

	bool ControlMsg(int requesttype, int request, int value,
			int index, char *bytes, int size, int timeout);

	/////////////////////////////
	// Combo functions

	bool GetConfiguration(unsigned char &cfg);
};

class BXEXPORT Interface
{
	Device &m_dev;
	int m_iface;
public:
	Interface(Device &dev, int iface);
	~Interface();
	bool SetAltInterface(int altSetting);
};

// Map of Endpoint numbers (not indexes) to endpoint descriptors
struct BXEXPORT EndpointPair
{
	unsigned char read;
	unsigned char write;
	EndpointDescriptor::EpType type;

	EndpointPair();
	bool IsTypeSet() const;
	bool IsComplete() const;
	bool IsBulk() const;
};

class BXEXPORT EndpointPairings : public std::vector<EndpointPair>
{
public:
	typedef std::vector<EndpointPair> base_type;
private:
	bool m_valid;
public:
	EndpointPairings(const std::vector<EndpointDescriptor*>& eps);
	~EndpointPairings();
	bool IsValid() const;
};

class BXEXPORT Match
{
private:
	std::vector<DeviceID> m_list;
	std::vector<DeviceID>::iterator m_iter;
public:
	// Due to USB libraries having different ownership ideas
	// about device IDs, Match objects must be constructed
	// with a device list.
	Match(DeviceList& devices,
	      int vendor, int product,
	      const char *busname = 0, const char *devname = 0);	
	~Match();

	// searches for next match, and if found, fills devid with
	// something you can pass on to DeviceDiscover, etc
	// returns true if next is found, false if no more
	bool next_device(Usb::DeviceID& devid);
};

}

#endif
