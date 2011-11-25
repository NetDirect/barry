///
/// \file	usbwrap_libusb_1_0.cc
///		USB API wrapper for libusb version 1.0
///

/*
    Copyright (C) 2005-2011, Chris Frey
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



#include "usbwrap_libusb_1_0.h"

#include "debug.h"
#include "data.h"
#include <errno.h>
#include <sstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifndef __DEBUG_MODE__
#define __DEBUG_MODE__
#endif
#include "debug.h"

namespace Usb {

// helper functions to make deleting pointers in maps and vectors easier
template<typename T> static void deletePtr(T* ptr)
{
	delete ptr;
}

template<typename K, typename T> static void deleteMapPtr(std::pair<K,T*> ptr)
{
	delete ptr.second;
}

// lookup table translating LIBUSB errors to standard Linux errors
static const struct {
	int libusb;
	int system;
} errorCodes[] = {
	{ LIBUSB_SUCCESS, 0 },
	{ LIBUSB_ERROR_IO, -EIO },
	{ LIBUSB_ERROR_INVALID_PARAM, -EINVAL },
	{ LIBUSB_ERROR_ACCESS, -EACCES },
	{ LIBUSB_ERROR_NO_DEVICE, -ENODEV },
	{ LIBUSB_ERROR_NOT_FOUND, -ENOENT },
	{ LIBUSB_ERROR_BUSY, -EBUSY },
	{ LIBUSB_ERROR_TIMEOUT, -ETIMEDOUT },
	{ LIBUSB_ERROR_OVERFLOW, -EOVERFLOW },
	{ LIBUSB_ERROR_PIPE, -EPIPE },
	{ LIBUSB_ERROR_INTERRUPTED, -EINTR },
	{ LIBUSB_ERROR_NO_MEM, -ENOMEM },
	{ LIBUSB_ERROR_NOT_SUPPORTED, -ENOSYS },
	// There isn't an errno.h value for generic errors, so
	// return success, which, for TranslateErrcode(), means error.
	{ LIBUSB_ERROR_OTHER, 0 }
};

static const int errorCodeCnt = sizeof(errorCodes) / sizeof(errorCodes[0]);


///////////////////////////////////////////////////////////////////////////////
// Global libusb library context
static libusb_context* libusbctx;

///////////////////////////////////////////////////////////////////////////////
// Static functions

std::string LibraryInterface::GetLastErrorString(int libusb_errcode)
{
	switch( libusb_errcode )
	{
	case LIBUSB_SUCCESS:
		return "Success";
	case LIBUSB_ERROR_IO:
		return "IO Error";
	case LIBUSB_ERROR_INVALID_PARAM:
		return "Invalid parameter";
	case LIBUSB_ERROR_ACCESS:
		return "Access";
	case LIBUSB_ERROR_NO_DEVICE:
		return "No device";
	case LIBUSB_ERROR_NOT_FOUND:
		return "Not found";
	case LIBUSB_ERROR_BUSY:
		return "Busy";
	case LIBUSB_ERROR_TIMEOUT:
		return "Timeout";
	case LIBUSB_ERROR_OVERFLOW:
		return "Overflow";
	case LIBUSB_ERROR_PIPE:
		return "Pipe";
	case LIBUSB_ERROR_INTERRUPTED:
		return "Interrupted";
	case LIBUSB_ERROR_NO_MEM:
		return "No memory";
	case LIBUSB_ERROR_NOT_SUPPORTED:
		return "Not supported";
	case LIBUSB_ERROR_OTHER:
		return "Other";
	default:
		return "Unknown LIBUSB error code";
	}
}

// Helper function to translate libusb error codes into more useful values
//
// Note that this function assumes that libusb_errcode contains an error.
// It is helpful enough to return 0 if libusb_errcode contains 0, but
// if it is a positive success value (such as for a read or write)
// it will still return 0, since it won't find a corresponding errno code.
//
// Since this function assumes that libusb_errcode is already an error,
// it also assumes the caller already *knows* that it is an error, and
// therefore a return of success is an "error" for this function. :-)
//
int LibraryInterface::TranslateErrcode(int libusb_errcode)
{
	for( int i = 0; i < errorCodeCnt; ++i ) {
		if( errorCodes[i].libusb == libusb_errcode )
			return errorCodes[i].system;
	}

	// default to 0 if unknown
	eout("Failed to translate libusb errorcode: " << libusb_errcode);
	return 0;
}

bool LibraryInterface::Init(int *libusb_errno)
{
	// if the environment variable LIBUSB_DEBUG is set, that
	// level value will be used instead of our 3 above...
	// if you need to *force* this to 3, call SetDataDump(true)
	// after Init()
	if( !libusbctx ) {
		int ret = libusb_init(&libusbctx);

		// store errno for user if possible
		if( libusb_errno )
			*libusb_errno = ret;

		// true on success
		return ret >= 0;
	}
	return true;
}

void LibraryInterface::Uninit()
{
	libusb_exit(libusbctx);
	libusbctx = NULL;
}

void LibraryInterface::SetDataDump(bool data_dump_mode)
{
	if( !libusbctx ) {
		Init();
	}
	if( !libusbctx ) {
		// Failed to init, can't do much but return
		dout("SetDataDump: Failed to initialise libusb");
		return;
	}
	if( data_dump_mode )
		libusb_set_debug(libusbctx, 3);
	else
		libusb_set_debug(libusbctx, 0);
}

///////////////////////////////////////////////////////////////////////////////
// DeviceIDImpl

DeviceIDImpl::DeviceIDImpl(libusb_device *dev)
	: m_dev(dev)
{
	libusb_ref_device(m_dev);

	// Libusb 1.0 doesn't provide busnames or filenames
	// so it's necessary to make some up.
	std::ostringstream formatter;
	formatter << "libusb1-"
		  << static_cast<int>(libusb_get_bus_number(m_dev));
	m_busname = formatter.str();

	formatter << "-"
		  << static_cast<int>(libusb_get_device_address(m_dev));
	m_filename = formatter.str();
}

DeviceIDImpl::~DeviceIDImpl()
{
	libusb_unref_device(m_dev);
}

///////////////////////////////////////////////////////////////////////////////
// DeviceID

DeviceID::DeviceID(DeviceIDImpl* impl)
	: m_impl(impl)
{
}

DeviceID::~DeviceID()
{
}


const char* DeviceID::GetBusName() const
{
	return m_impl->m_busname.c_str();
}

uint16_t DeviceID::GetNumber() const
{
	return libusb_get_device_address(m_impl->m_dev);
}

const char* DeviceID::GetFilename() const
{
	return m_impl->m_filename.c_str();
}

uint16_t DeviceID::GetIdProduct() const
{
	int ret = PRODUCT_UNKNOWN;
	struct libusb_device_descriptor desc;
	int err = libusb_get_device_descriptor(m_impl->m_dev, &desc);
	if( err == 0 )
		ret = desc.idProduct;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// DeviceList

DeviceList::DeviceList()
	: m_impl(new DeviceListImpl())
{
	m_impl->m_list = NULL;
	m_impl->m_listcnt = 0;

	m_impl->m_listcnt = libusb_get_device_list(libusbctx, &m_impl->m_list);
	if( m_impl->m_listcnt < 0 ) {
		throw Error("Failed to get device list");
	}

	for( int i = 0; i < m_impl->m_listcnt; ++i ) {
		// Add the device to the list of devices
		DeviceID devID(new DeviceIDImpl(m_impl->m_list[i]));
		m_impl->m_devices.push_back(devID);
	}
}

DeviceList::~DeviceList()
{
	if( m_impl->m_list ) {
		libusb_free_device_list(m_impl->m_list, 1);
	}
}

std::vector<DeviceID> DeviceList::MatchDevices(int vendor, int product,
					    const char *busname, const char *devname)
{
	std::vector<DeviceID> ret;
	int err;

	std::vector<DeviceID>::iterator iter = m_impl->m_devices.begin();

	for( ; iter != m_impl->m_devices.end() ; ++iter ) {
		struct libusb_device* dev = iter->m_impl->m_dev;

		// only search on given bus
		if( busname && atoi(busname) != libusb_get_bus_number(dev) )
			continue;

		// search for specific device
		if( devname && atoi(devname) != libusb_get_device_address(dev) )
			continue;

		struct libusb_device_descriptor desc;
		err = libusb_get_device_descriptor(dev, &desc);
		if( err ) {
			dout("Failed to get device descriptor: " << err);
			continue;
		}

		// is there a match?
		if( desc.idVendor == vendor &&
		    ( desc.idProduct == product || 
		      product == PRODUCT_ANY )) {
			ret.push_back(*iter);
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Device

Device::Device(const Usb::DeviceID& id, int timeout)
	: m_id(id),
	m_timeout(timeout)
{
	dout("libusb_open(" << std::dec << id.m_impl.get() << ")");
	if( !&(*id.m_impl) )
		throw Error("invalid USB device ID");
	m_handle.reset(new DeviceHandle());
	int err = libusb_open(id.m_impl->m_dev, &(m_handle->m_handle));
	m_lasterror = err;
	if( err )
		throw Error("open failed");
}

Device::~Device()
{
	dout("libusb_close(" << std::dec << m_handle->m_handle << ")");
	libusb_close(m_handle->m_handle);
}

bool Device::SetConfiguration(unsigned char cfg)
{
	dout("libusb_set_configuration(" << std::dec << m_handle->m_handle << ", 0x" << std::hex << (unsigned int) cfg << ")");
	int ret = libusb_set_configuration(m_handle->m_handle, cfg);
	m_lasterror = ret;
	return ret >= 0;
}

bool Device::ClearHalt(int ep)
{
	dout("libusb_clear_halt(" << std::dec << m_handle->m_handle << ", 0x" << std::hex << ep << ")");
	int ret = libusb_clear_halt(m_handle->m_handle, ep);
	m_lasterror = ret;
	return ret >= 0;
}

bool Device::Reset()
{
	dout("libusb_reset_device(" << std::dec << m_handle->m_handle << ")");
	int ret = libusb_reset_device(m_handle->m_handle);
	m_lasterror = ret;
	return ret == 0;
}

bool Device::BulkRead(int ep, Barry::Data &data, int timeout)
{
	ddout("BulkRead to endpoint 0x" << std::hex << ep << ":\n" << data);
	int ret;
	do {
		int transferred = 0;
		data.QuickZap();
		ret = libusb_bulk_transfer(m_handle->m_handle,
                        ep |= LIBUSB_ENDPOINT_IN,
		        data.GetBuffer(), data.GetBufSize(),
		        &transferred,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != LIBUSB_ERROR_INTERRUPTED ) {
			m_lasterror = ret;
			// only notify of a timeout if no data was transferred,
			// otherwise treat it as success.
			if( ret == LIBUSB_ERROR_TIMEOUT ) {
				if( transferred == 0 )
					throw Timeout(ret, "Timeout in BulkRead");
				else
					dout("Read timed out with some data transferred... possible partial read");
			}
			else if( ret != LIBUSB_ERROR_TIMEOUT ) {
				std::ostringstream oss;
				oss << "Error in libusb_bulk_tranfer("
				    << m_handle->m_handle << ", "
				    << ep << ", buf, "
				    << data.GetBufSize() << ", "
				    << transferred << ", "
				    << (timeout == -1 ? m_timeout : timeout)
				    << ")";
				throw Error(ret, oss.str());
			}
		}
		if( transferred != 0 )
			data.ReleaseBuffer(transferred);

	} while( ret == -LIBUSB_ERROR_INTERRUPTED );

	return ret >= 0;
}

bool Device::BulkWrite(int ep, const Barry::Data &data, int timeout)
{
	ddout("BulkWrite to endpoint 0x" << std::hex << ep << ":\n" << data);
	int ret;
	do {
		int transferred;
		ret = libusb_bulk_transfer(m_handle->m_handle,
                        ep | LIBUSB_ENDPOINT_OUT,
			const_cast<unsigned char*>(data.GetData()),
			data.GetSize(),
		        &transferred,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != LIBUSB_ERROR_INTERRUPTED ) {
			m_lasterror = ret;
			// only notify of a timeout if no data was transferred,
			// otherwise treat it as success.
			if( ret == LIBUSB_ERROR_TIMEOUT && transferred == 0 )
				throw Timeout(ret, "Timeout in BulkWrite");
			else if( ret != LIBUSB_ERROR_TIMEOUT )
				throw Error(ret, "Error in BulkWrite");
		}
		if( ret >= 0 &&
		    (unsigned int)transferred != data.GetSize() ) {
			dout("Failed to write all data on ep: " << ep <<
			     " attempted to write: " << data.GetSize() << 
			     " but only wrote: " << transferred);
			throw Error("Failed to perform a complete write");
		}

	} while( ret == -LIBUSB_ERROR_INTERRUPTED );

	return ret >= 0;
}

bool Device::BulkWrite(int ep, const void *data, size_t size, int timeout)
{
#ifdef __DEBUG_MODE__
	Barry::Data dump(data, size);
	ddout("BulkWrite to endpoint 0x" << std::hex << ep << ":\n" << dump);
#endif	
	int ret;
	do {
		int transferred;
		ret = libusb_bulk_transfer(m_handle->m_handle,
                        ep | LIBUSB_ENDPOINT_OUT,
			(unsigned char*)const_cast<void*>(data),
			size,
		        &transferred,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != LIBUSB_ERROR_INTERRUPTED ) {
			m_lasterror = ret;
			// only notify of a timeout if no data was transferred,
			// otherwise treat it as success.
			if( ret == LIBUSB_ERROR_TIMEOUT && transferred == 0 )
				throw Timeout(ret, "Timeout in BulkWrite (2)");
			else if( ret != LIBUSB_ERROR_TIMEOUT )
				throw Error(ret, "Error in BulkWrite (2)");
		}
		if( ret >= 0 && (unsigned int)transferred != size ) {
			dout("Failed to write all data on ep: " << ep <<
			     " attempted to write: " << size << 
			     " but only wrote: " << transferred);
			throw Error("Failed to perform a complete write");
		}

	} while( ret == -LIBUSB_ERROR_INTERRUPTED );

	return ret >= 0;
}

bool Device::InterruptRead(int ep, Barry::Data &data, int timeout)
{
	ddout("InterruptRead to endpoint 0x" << std::hex << ep << ":\n" << data);
	int ret;
	do {
		int transferred = 0;
		data.QuickZap();
		ret = libusb_interrupt_transfer(m_handle->m_handle,
                        ep | LIBUSB_ENDPOINT_IN,
		        data.GetBuffer(), data.GetBufSize(),
		        &transferred,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != LIBUSB_ERROR_INTERRUPTED ) {
			m_lasterror = ret;
			// only notify of a timeout if no data was transferred,
			// otherwise treat it as success.
			if( ret == LIBUSB_ERROR_TIMEOUT ) {
				if( transferred == 0 )
					throw Timeout(ret, "Timeout in InterruptRead");
				else
					dout("Read timed out with some data transferred... possible partial read");
			}
			else if( ret != LIBUSB_ERROR_TIMEOUT )
				throw Error(ret, "Error in InterruptRead");
		}
		if( transferred != 0 )
			data.ReleaseBuffer(transferred);

	} while( ret == -LIBUSB_ERROR_INTERRUPTED );

	return ret >= 0;

}

bool Device::InterruptWrite(int ep, const Barry::Data &data, int timeout)
{
	ddout("InterruptWrite to endpoint 0x" << std::hex << ep << ":\n" << data);
	int ret;
	do {
		int transferred;
		ret = libusb_interrupt_transfer(m_handle->m_handle,
			ep | LIBUSB_ENDPOINT_OUT,
			const_cast<unsigned char*>(data.GetData()),
			data.GetSize(),
		        &transferred,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != LIBUSB_ERROR_INTERRUPTED ) {
			m_lasterror = ret;
			// only notify of a timeout if no data was transferred,
			// otherwise treat it as success.
			if( ret == LIBUSB_ERROR_TIMEOUT && transferred == 0 )
				throw Timeout(ret, "Timeout in InterruptWrite");
			else if( ret != LIBUSB_ERROR_TIMEOUT )
				throw Error(ret, "Error in InterruptWrite");
		}
		if( ret >= 0 && 
		    (unsigned int)transferred != data.GetSize() ) {
			dout("Failed to write all data on ep: " << ep <<
			     " attempted to write: " << data.GetSize() << 
			     " but only wrote: " << transferred);
			throw Error("Failed to perform a complete write");
		}

	} while( ret == -LIBUSB_ERROR_INTERRUPTED );

	return ret >= 0;
}

//
// BulkDrain
//
/// Reads anything available on the given endpoint, with a low timeout,
/// in order to clear any pending reads.
///
void Device::BulkDrain(int ep, int timeout)
{
	try {
		Barry::Data data;
		while( BulkRead(ep, data, timeout) )
		;
	}
	catch( Usb::Error & ) {}
}

//
// GetConfiguration
//
/// Determines the currently selected USB configuration, returning it
/// in the cfg argument.
/// If unsuccessful, returns false.
///
bool Device::GetConfiguration(unsigned char &cfg)
{
	int config = 0;
	int result = libusb_get_configuration(m_handle->m_handle, &config);
	if( result >= 0 )
		cfg = config;
	m_lasterror = result;
	return result >= 0;
}

// Returns the current power level of the device, or 0 if unknown
int Device::GetPowerLevel()
{
	struct libusb_config_descriptor* cfg = NULL;
	int ret = 0;
	int result = libusb_get_active_config_descriptor(m_id.m_impl->m_dev, &cfg);
	m_lasterror = result;
	if( result == 0 ) {
		ret = cfg->MaxPower;
	}

	if( cfg )
		libusb_free_config_descriptor(cfg);

	return ret;
}

bool Device::IsAttachKernelDriver(int iface, std::string &name)
{
	int ret;

	ret = libusb_kernel_driver_active(m_handle->m_handle, iface);
	if (ret == 0) {
		dout("interface (" << m_handle->m_handle << ", 0x" << std::hex << iface
			<< ") already claimed by a driver of unknown name.");
		name.clear();
		return true;
	}

	return false;
}

// Requests that the kernel driver is detached, returning false on failure
bool Device::DetachKernelDriver(int iface)
{
	int result = libusb_detach_kernel_driver(m_handle->m_handle, iface);
	m_lasterror = result;
	return result >= 0;
}

// Sends a control message to the device, returning false on failure
bool Device::ControlMsg(int requesttype, int request, int value,
			int index, char *bytes, int size, int timeout)
{
	int result = libusb_control_transfer(m_handle->m_handle,
					     requesttype, request, value, index,
					     (unsigned char*)bytes, size, timeout);
	m_lasterror = result;
	return result >= 0;
}

int Device::FindInterface(int ifaceClass)
{
	struct libusb_config_descriptor* cfg = NULL;
	int ret = libusb_get_active_config_descriptor(m_id.m_impl->m_dev, &cfg);
	if( ret == 0 ) {
		for( int i = 0; cfg->interface && i < cfg->bNumInterfaces; ++i ) {
			const struct libusb_interface& iface = cfg->interface[i];
			for( int j = 0;
			     iface.altsetting && j < iface.num_altsetting;
			     ++j ) {
				const struct libusb_interface_descriptor& id = 
					iface.altsetting[j];
				if( id.bInterfaceClass == ifaceClass )
					return id.bInterfaceNumber;
			}
		}
	}

	if( cfg )
		libusb_free_config_descriptor(cfg);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Interface

Interface::Interface(Device &dev, int iface)
	: m_dev(dev), m_iface(iface)
{
	dout("libusb_claim_interface(" << dev.GetHandle()->m_handle << ", 0x" << std::hex << iface << ")");
	int ret = libusb_claim_interface(dev.GetHandle()->m_handle, iface);
	if( ret < 0 )
		throw Error(ret, "claim interface failed");
}

Interface::~Interface()
{
	dout("libusb_release_interface(" << m_dev.GetHandle()->m_handle << ", 0x" << std::hex << m_iface << ")");
	libusb_release_interface(m_dev.GetHandle()->m_handle, m_iface);
}


//
// SetAltInterface
//
/// Sets the currently selected USB alternate setting of the current interface.
/// The iface parameter passed in should be a value specified
/// in the bAlternateSetting descriptor field.
/// If unsuccessful, returns false.
///
bool Interface::SetAltInterface(int altSetting)
{
	int result = libusb_set_interface_alt_setting(
		m_dev.GetHandle()->m_handle,
		m_iface,
		altSetting);
	m_dev.SetLastError(result);
	return result >= 0;
}



//////////////////////////////////////////////////////////////////
// DeviceDescriptor

DeviceDescriptor::DeviceDescriptor(DeviceID& devid)
	: m_impl(new DeviceDescriptorImpl())
{
	if( !devid.m_impl.get() ) {
		dout("DeviceDescriptor: empty devid");
		return;
	}
	m_impl->m_devid = devid;
	int ret = libusb_get_device_descriptor(devid.m_impl->m_dev, &m_impl->m_desc);
	if( ret != 0 ) {
		dout("Failed to read device descriptor with err: " << ret);
		return;
	}
	dout("device_desc loaded"
	     << "\nbLength: " << std::dec << (unsigned int) m_impl->m_desc.bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned int) m_impl->m_desc.bDescriptorType
	     << "\nbcdUSB: 0x" << std::hex << (unsigned int) m_impl->m_desc.bcdUSB
	     << "\nbDeviceClass: " << std::dec << (unsigned int) m_impl->m_desc.bDeviceClass
	     << "\nbDeviceSubClass: " << std::dec << (unsigned int) m_impl->m_desc.bDeviceSubClass
	     << "\nbDeviceProtocol: " << std::dec << (unsigned int) m_impl->m_desc.bDeviceProtocol
	     << "\nbMaxPacketSize0: " << std::dec << (unsigned int) m_impl->m_desc.bMaxPacketSize0
	     << "\nidVendor: 0x" << std::hex << (unsigned int) m_impl->m_desc.idVendor
	     << "\nidProduct: 0x" << std::hex << (unsigned int) m_impl->m_desc.idProduct
	     << "\nbcdDevice: 0x" << std::hex << (unsigned int) m_impl->m_desc.bcdDevice
	     << "\niManufacturer: " << std::dec << (unsigned int) m_impl->m_desc.iManufacturer
	     << "\niProduct: " << std::dec << (unsigned int) m_impl->m_desc.iProduct
	     << "\niSerialNumber: " << std::dec << (unsigned int) m_impl->m_desc.iSerialNumber
	     << "\nbNumConfigurations: " << std::dec << (unsigned int) m_impl->m_desc.bNumConfigurations
	     << "\n"
	);

	// Create all the configs
	for( int i = 0; i < m_impl->m_desc.bNumConfigurations; ++i ) {
		std::auto_ptr<ConfigDescriptor> ptr(new ConfigDescriptor(*this, i));
		(*this)[ptr->GetNumber()] = ptr.get();
		ptr.release();
	}
}

DeviceDescriptor::~DeviceDescriptor()
{
	// Delete any pointers in the vector
	std::for_each(begin(),
		      end(),
		      deleteMapPtr<int, ConfigDescriptor>);
}

///////////////////////////////////////////////////////////////////
// ConfigDescriptor

ConfigDescriptor::ConfigDescriptor(DeviceDescriptor& dev, int cfgnumber)
	: m_impl(new ConfigDescriptorImpl())
{
	m_impl->m_desc = NULL;
	int ret = libusb_get_config_descriptor(dev.m_impl->m_devid.m_impl->m_dev,
					       cfgnumber, &(m_impl->m_desc));
	if( ret != 0 ) {
		dout("Failed to read config descriptor with err: " << ret);
		return;
	}

	dout("  config_desc #" << std::dec << cfgnumber << " loaded"
	     << "\nbLength: " << std::dec << (unsigned int) m_impl->m_desc->bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned int) m_impl->m_desc->bDescriptorType
	     << "\nwTotalLength: " << std::dec << (unsigned int) m_impl->m_desc->wTotalLength
	     << "\nbNumInterfaces: " << std::dec << (unsigned int) m_impl->m_desc->bNumInterfaces
	     << "\nbConfigurationValue: " << std::dec << (unsigned int) m_impl->m_desc->bConfigurationValue
	     << "\niConfiguration: " << std::dec << (unsigned int) m_impl->m_desc->iConfiguration
	     << "\nbmAttributes: 0x" << std::hex << (unsigned int) m_impl->m_desc->bmAttributes
	     << "\nMaxPower: " << std::dec << (unsigned int) m_impl->m_desc->MaxPower
	     << "\n"
		);

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( m_impl->m_desc->extra ) {
		dout("while parsing config descriptor, found a block of extra descriptors:");
		Barry::Data data(m_impl->m_desc->extra, m_impl->m_desc->extra_length);
		dout(data);
	}

	// Create all the interfaces
	for( int i = 0; i < m_impl->m_desc->bNumInterfaces; ++i ) {
		const struct libusb_interface* interface = &(m_impl->m_desc->interface[i]);
		if( !interface->altsetting ) {
			dout("ConfigDescriptor: empty altsetting");
			// some devices are buggy and return a higher bNumInterfaces
			// than the number of interfaces available... in this case
			// we just skip and continue
			continue;
		}
		for( int j = 0; j < interface->num_altsetting; ++j ) {
			std::auto_ptr<InterfaceDescriptor> ptr(
				new InterfaceDescriptor(*this, i, j));
			(*this)[ptr->GetNumber()] = ptr.get();
			ptr.release();
		}
	}
}

ConfigDescriptor::~ConfigDescriptor()
{
	// Delete any pointers in the vector
	std::for_each(begin(),
		      end(),
		      deleteMapPtr<int, InterfaceDescriptor>);
	if( m_impl->m_desc ) {
		libusb_free_config_descriptor(m_impl->m_desc);
		m_impl->m_desc = NULL;
	}
}

uint8_t ConfigDescriptor::GetNumber() const {
	if( !m_impl->m_desc )
		// Return an invalid config number
		return 0;
	return m_impl->m_desc->bConfigurationValue;
}

/////////////////////////////////////////////////////////////////////////
// InterfaceDescriptor

InterfaceDescriptor::InterfaceDescriptor(ConfigDescriptor& cfgdesc,
					 int interface, int altsetting)
	: m_impl(new InterfaceDescriptorImpl())
{
	m_impl->m_desc = NULL;

	// Find the descriptor
	m_impl->m_desc =
		&(cfgdesc.m_impl->m_desc
		     ->interface[interface]
		  .altsetting[altsetting]);
	dout("    interface_desc #" << std::dec << interface << " loaded"
	     << "\nbLength: " << std::dec << (unsigned) m_impl->m_desc->bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned) m_impl->m_desc->bDescriptorType
	     << "\nbInterfaceNumber: " << std::dec << (unsigned) m_impl->m_desc->bInterfaceNumber
	     << "\nbAlternateSetting: " << std::dec << (unsigned) m_impl->m_desc->bAlternateSetting
	     << "\nbNumEndpoints: " << std::dec << (unsigned) m_impl->m_desc->bNumEndpoints
	     << "\nbInterfaceClass: " << std::dec << (unsigned) m_impl->m_desc->bInterfaceClass
	     << "\nbInterfaceSubClass: " << std::dec << (unsigned) m_impl->m_desc->bInterfaceSubClass
	     << "\nbInterfaceProtocol: " << std::dec << (unsigned) m_impl->m_desc->bInterfaceProtocol
	     << "\niInterface: " << std::dec << (unsigned) m_impl->m_desc->iInterface
	     << "\n"
		);

	if( !m_impl->m_desc->endpoint ) {
		dout("InterfaceDescriptor: empty interface pointer");
		return;
	}

	// Create all the endpoints
	for( int i = 0; i < m_impl->m_desc->bNumEndpoints; ++i ) {
		std::auto_ptr<EndpointDescriptor> ptr (
			new EndpointDescriptor(*this, i));
		push_back(ptr.get());
		ptr.release();
	}

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( m_impl->m_desc->extra ) {
		dout("while parsing interface descriptor, found a block of extra descriptors:");
		Barry::Data data(m_impl->m_desc->extra, m_impl->m_desc->extra_length);
		dout(data);
	}
}

InterfaceDescriptor::~InterfaceDescriptor()
{
	// Delete any pointers in the vector
	std::for_each(begin(),
		      end(),
		      deletePtr<EndpointDescriptor>);
}

uint8_t InterfaceDescriptor::GetClass() const
{
	return m_impl->m_desc->bInterfaceClass;
}

uint8_t InterfaceDescriptor::GetNumber() const
{
	if( !m_impl->m_desc )
		// Return an invalid interface number
		return 0;
	return m_impl->m_desc->bInterfaceNumber;
}

uint8_t InterfaceDescriptor::GetAltSetting() const
{
	if( !m_impl->m_desc )
		// Return an invalid setting number
		return 0;
	return m_impl->m_desc->bAlternateSetting;
}

/////////////////////////////////////////////////////////////////////////////////
// EndpointDescriptor

EndpointDescriptor::EndpointDescriptor(InterfaceDescriptor& intdesc, int endpoint)
	: m_impl(new EndpointDescriptorImpl()),
	  m_read(false),
	  m_addr(0),
	  m_type(InvalidType)
{
	// Copy the descriptor
	m_impl->m_desc = &(intdesc.m_impl->m_desc->endpoint[endpoint]);

	dout("      endpoint_desc #" << std::dec << endpoint << " loaded"
	     << "\nbLength: " << std::dec << (unsigned ) m_impl->m_desc->bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned ) m_impl->m_desc->bDescriptorType
	     << "\nbEndpointAddress: 0x" << std::hex << (unsigned ) m_impl->m_desc->bEndpointAddress
	     << "\nbmAttributes: 0x" << std::hex << (unsigned ) m_impl->m_desc->bmAttributes
	     << "\nwMaxPacketSize: " << std::dec << (unsigned ) m_impl->m_desc->wMaxPacketSize
	     << "\nbInterval: " << std::dec << (unsigned ) m_impl->m_desc->bInterval
	     << "\nbRefresh: " << std::dec << (unsigned ) m_impl->m_desc->bRefresh
	     << "\nbSynchAddress: " << std::dec << (unsigned ) m_impl->m_desc->bSynchAddress
	     << "\n"
		);
	// Set up variables
	m_read = ((m_impl->m_desc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == 
		LIBUSB_ENDPOINT_IN);
	m_addr = (m_impl->m_desc->bEndpointAddress & LIBUSB_ENDPOINT_ADDRESS_MASK);
	int type = (m_impl->m_desc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK);
	m_type = static_cast<Usb::EndpointDescriptor::EpType>(type);

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( m_impl->m_desc->extra ) {
		dout("while parsing endpoint descriptor, found a block of extra descriptors:");
		Barry::Data data(m_impl->m_desc->extra, m_impl->m_desc->extra_length);
		dout(data);
	}
}

EndpointDescriptor::~EndpointDescriptor()
{
}

} // namespace Usb

