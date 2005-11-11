///
/// \file	usbwrap.h
///		USB API wrapper
///
// FIXME - add copyright notices


#ifndef __SB_USBWRAP_H__
#define __SB_USBWRAP_H__

#include <libusb.h>
#include <vector>
#include <stdexcept>

#define USBWRAP_DEFAULT_TIMEOUT	1000

class Data;

namespace Usb {

class UsbError : public std::runtime_error
{
public:
	UsbError(const std::string &str) : std::runtime_error(str) {}
};


class Match
{
	libusb_match_handle_t *m_match;
	int m_lasterror;
public:
	Match(int vendor, int product)
		: m_match(0)
	{
		if( libusb_match_devices_by_vendor(&m_match, vendor, product) < 0 )
			throw UsbError("match failed");
	}

	~Match()
	{
		libusb_free_match(m_match);
	}

	bool next_device(libusb_device_id_t *devid)
	{
		m_lasterror = libusb_match_next_device(m_match, devid);
		return m_lasterror >= 0;
	}
};




class Device;

// This class behaves a lot like std::auto_ptr<>, in that only one
// object can ever own a given libusb_io_handle_t at a time.  The
// copy constructors and operator=() make it safe to store in a vector.
class IO
{
	Device *m_dev;
	mutable libusb_io_handle_t *m_handle;

	// helpers
	void cleanup();

	IO(Device *dev, libusb_io_handle_t *handle);
public:
	IO(Device &dev, libusb_io_handle_t *handle);

	// copyable... the copied-to object will contain the handle,
	// and other will be invalidated
	IO(const IO &other);
	IO& operator=(const IO &other);

	// will cancel if not complete, and will always free if valid
	~IO();


	// operators
	bool operator==(libusb_io_handle_t *h) const { return m_handle == h; }

	// if valid, this object contains a pending usb io transfer handle...
	// when compeleted, canceled, or freed, this will return false
	bool IsValid() const		{ return m_handle != 0; }
	void Forget()			{ m_handle = 0; }
	void Free()			{ cleanup(); }

	// API wrappers
	bool IsCompleted() const;
	int GetStatus() const;
	int GetSize() const;
	int GetReqSize() const;
	int GetEndpoint() const;
	Device& GetDevice() const	{ return *m_dev; }
	unsigned char * GetData();
	const unsigned char * GetData() const;

	// static helpers for various states
	static IO InvalidIO()		{ return IO(0, 0); }
};


// Const IO - used by Device when it wishes to safely expose its list
// of tracked IO handles, without letting the caller take ownership
class CIO
{
	const IO &m_io;

public:
	CIO(const IO &io) : m_io(io) {}

	// operators
	bool operator==(libusb_io_handle_t *h) const
		{ return m_io.operator==(h); }

	// if valid, this object contains a pending usb io transfer handle...
	// when compeleted, canceled, or freed, this will return false
	bool IsValid() const		{ return m_io.IsValid(); }

	// API wrappers
	bool IsCompleted() const	{ return m_io.IsCompleted(); }
	int GetStatus() const		{ return m_io.GetStatus();}
	int GetSize() const		{ return m_io.GetSize(); }
	int GetReqSize() const		{ return m_io.GetReqSize();}
	int GetEndpoint() const		{ return m_io.GetEndpoint(); }
	Device& GetDevice() const	{ return m_io.GetDevice(); }
	const unsigned char * GetData() const { return m_io.GetData(); }
};



class Device
{
public:
	typedef std::vector<IO> io_list_type;

private:
	libusb_device_id_t m_id;
	libusb_dev_handle_t *m_handle;

	io_list_type m_ios;

	int m_timeout;
	int m_lasterror;

public:
	Device(libusb_device_id_t id)
		: m_id(id), m_timeout(USBWRAP_DEFAULT_TIMEOUT)
	{
		if( libusb_open(m_id, &m_handle) < 0 )
			throw UsbError("open failed");
	}

	~Device()
	{
		ClearIO();		// remove all pending IO before close
		libusb_close(m_handle);
	}

	/////////////////////////////
	// Data access

	libusb_device_id_t GetID() const { return m_id; }
	libusb_dev_handle_t * GetHandle() const { return m_handle; }
	int GetLastError() const { return m_lasterror; }


	/////////////////////////////
	// Device manipulation

	bool SetConfiguration(unsigned char cfg);
	bool Reset();


	/////////////////////////////
	// IO functions

	// perform async io and track the handles inside Device
	void TrackBulkRead(int ep, Data &data);
	void TrackBulkWrite(int ep, const Data &data);
	void TrackInterruptRead(int ep, Data &data);
	void TrackInterruptWrite(int ep, const Data &data);
	IO PollCompletions();		//< extracts from tracking list!
	void ClearIO()			{ m_ios.clear(); }
	void ClearIO(int index);

	// perform async io and don't track handles in Device, but
	// return the IO handle immediately for the caller to deal with
	IO ABulkRead(int ep, Data &data);
	IO ABulkWrite(int ep, const Data &data);
	IO ABulkWrite(int ep, const void *data, size_t size);
	IO AInterruptRead(int ep, Data &data);
	IO AInterruptWrite(int ep, const Data &data);

	// tracking list access
	int GetIOCount() const		{ return m_ios.size(); }
	CIO GetIO(int index) const	{ return CIO(m_ios[index]); }
};

class Interface
{
	Device &m_dev;
	int m_iface;
public:
	Interface(Device &dev, int iface)
		: m_dev(dev), m_iface(iface)
	{
		if( libusb_claim_interface(dev.GetHandle(), iface) < 0 )
			throw UsbError("claim interface failed");
	}

	~Interface()
	{
		libusb_release_interface(m_dev.GetHandle(), m_iface);
	}
};

} // namespace Usb

#endif

