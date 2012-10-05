///
/// \file	router.h
///		Support classes for the pluggable socket routing system.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_ROUTER_H__
#define __BARRY_ROUTER_H__

#include "dll.h"
#include <stdint.h>
#include <map>
#include <tr1/memory>
#include <stdexcept>
#include <pthread.h>
#include "dataqueue.h"
#include "error.h"
#include "usbwrap.h"

namespace Barry {

class DataHandle;

class BXEXPORT SocketRoutingQueue
{
	friend class DataHandle;

public:
	// When registering interest in socket packets
	// this type is used to indicate what type of
	// packets are desired.
	enum InterestType
	{
		DataPackets = 0x1,
		SequencePackets = 0x2,
		SequenceAndDataPackets = DataPackets | SequencePackets
	};

	// Interface class for socket data callbacks
	// See RegisterInterest() for more information.
	class BXEXPORT SocketDataHandler
	{
	public:
		// Called when data is received on the socket
		// for which interest has been registered.
		//
		// The lifetime of the data parameter is only valid
		// for the duration of this method call.
		virtual void DataReceived(Data& data) = 0;

		// Called when an error has occured on the socket
		// for which interest has been registered.
		//
		// The lifetime of the error parameter is only valid
		// for the lifetime of this method call.
		virtual void Error(Barry::Error &error);

		virtual ~SocketDataHandler();
	};

	typedef std::tr1::shared_ptr<SocketDataHandler> SocketDataHandlerPtr;

	// Simple wrapper template class for SocketDataHandler which provides a basic data recieved callback
	template<typename T> class SimpleSocketDataHandler : public SocketDataHandler
	{
		void (*m_callback)(T&, Data*);
		T& m_context;
	public:
		SimpleSocketDataHandler<T>(T& context, void (*callback)(T& context, Data* data))
			: m_callback(callback)
			, m_context(context)
		{}
		virtual void DataReceived(Data& data)
		{
			m_callback(m_context, &data);
		}
	};

	struct QueueEntry
	{
		SocketDataHandlerPtr m_handler;
		DataQueue m_queue;
		InterestType m_type;

		QueueEntry(SocketDataHandlerPtr h, InterestType t)
			: m_handler(h)
			, m_type(t)
			{}
	};
	typedef std::tr1::shared_ptr<QueueEntry>	QueueEntryPtr;
	typedef uint16_t				SocketId;
	typedef std::map<SocketId, QueueEntryPtr>	SocketQueueMap;

private:
	Usb::Device * volatile m_dev;
	volatile int m_writeEp, m_readEp;

	volatile bool m_interest; // true if at least one socket has an interest.
				// used to optimize the reading

	mutable pthread_mutex_t m_mutex;// controls access to local data, but not
				// DataQueues, as they have their own
				// locking per queue

	pthread_mutex_t m_readwaitMutex;
	pthread_cond_t m_readwaitCond;
	bool m_seen_usb_error;
	SocketDataHandlerPtr m_usb_error_dev_callback;

	DataQueue m_free;
	DataQueue m_default;
	SocketQueueMap m_socketQueues;

	int m_timeout;

	// thread state
	pthread_t m_usb_read_thread;
	volatile bool m_continue_reading;// set to true when the thread is created,
				// then set to false in the destructor
				// to signal the end of the thread
				// and handle the join

protected:
	// Provides a method of returning a buffer to the free queue
	// after processing.  The DataHandle class calls this automatically
	// from its destructor.
	void ReturnBuffer(Data *buf);

	// Helper function to add a buffer to a socket queue
	// Returns false if no queue is available for that socket
	// Also empties the DataHandle on success.
	bool QueuePacket(SocketId socket, DataHandle &buf);
	bool QueuePacket(DataQueue &queue, DataHandle &buf);
	bool RouteOrQueuePacket(SocketId socket, DataHandle &buf);

	// Thread function for the simple read behaviour... thread is
	// created in the SpinoffSimpleReadThread() member below.
	static void *SimpleReadThread(void *userptr);

	void DumpSocketQueue(SocketId socket, const DataQueue &dq);

public:
	SocketRoutingQueue(int prealloc_buffer_count = 4,
		int default_read_timeout = USBWRAP_DEFAULT_TIMEOUT);
	~SocketRoutingQueue();

	//
	// data access
	//
	int GetWriteEp() const { return m_writeEp; }
	int GetReadEp() const { return m_readEp; }


	// These functions connect the router to an external Usb::Device
	// object.  Normally this is handled automatically by the
	// Controller class, but are public here in case they are needed.
	//
	// If DoRead encounters an error, it sets a flag and stops
	// reading.  To recover, you should handle the Error() call in
	// the callback, fix the USB device, and then call
	// ClearUsbError() to clear the flag.
	//
	void SetUsbDevice(Usb::Device *dev, int writeEp, int readEp,
		SocketDataHandlerPtr callback = SocketDataHandlerPtr());
	void ClearUsbDevice();
	bool UsbDeviceReady();
	Usb::Device* GetUsbDevice() { return m_dev; }
	void ClearUsbError();


	// This class starts out with no buffers, and will grow one buffer
	// at a time if needed.  Call this to allocate count buffers
	// all at once and place them on the free queue.
	void AllocateBuffers(int count);

	// Returns the data for the next unregistered socket.
	// Blocks until timeout or data is available.
	// Returns false (or null pointer) on timeout and no data.
	// With the return version of the function, there is no
	// copying performed.
	//
	// Timeout is in milliseconds.  Default timeout set by constructor
	// is used if set to -1.
	bool DefaultRead(Data &receive, int timeout = -1);
	DataHandle DefaultRead(int timeout = -1);

	// Internal registration of interest in data from a certain socket,
	// filtered by a type.  NOTE: most code should not use this function,
	// but use RegisterInterest() instead, since registering with the
	// wrong type could cause Socket::SyncSend() to malfunction.
	void RegisterInterestAndType(SocketId socket,
		SocketDataHandlerPtr handler, InterestType type);

	// Register an interest in data from a certain socket.  To read
	// from that socket, use the SocketRead() function from then on.
	// Any non-registered socket goes in the default queue
	// and must be read by DefaultRead()
	// If not null, handler is called when new data is read.  It will
	// be called in the same thread instance that DoRead() is called from.
	// Handler is passed the DataQueue Data object, and so no
	// copying is done.  Once the handler returns, the data is
	// considered processed and not added to the interested queue,
	// but instead returned to m_free.
	//
	// This simply calls RegisterInterestAndType(socket,handler,type)
	// with type set to SequenceAndDataPackets.  Most code should use
	// this function.
	void RegisterInterest(SocketId socket, SocketDataHandlerPtr handler);

	// Unregisters interest in data from the given socket, and discards
	// any existing data in its interest queue.  Any new incoming data
	// for this socket will be placed in the default queue.
	void UnregisterInterest(SocketId socket);

	// Changes the type of data that a client is interested in for a
	// certain socket.
	// Interest in the socket must have previously been registered by a
	// call to RegisterInterest() or RegisterInterestAndType().
	void ChangeInterest(SocketId socket, InterestType type);

	// Reads data from the interested socket cache.  Can only read
	// from sockets that have been previously registered.
	// Blocks until timeout or data is available.
	// Returns false (or null pointer) on timeout and no data.
	// With the return version of the function, there is no
	// copying performed.
	//
	// Timeout is in milliseconds.  Default timeout set by constructor
	// is used if set to -1.
	bool SocketRead(SocketId socket, Data &receive, int timeout = -1);
	DataHandle SocketRead(SocketId socket, int timeout = -1);

	// Returns true if data is available for that socket.
	bool IsAvailable(SocketId socket) const;

	// Called by the application's "read thread" to read the next usb
	// packet and route it to the correct queue.  Returns after every
	// read, even if a handler is associated with a queue.
	// Note: this function is safe to call before SetUsbDevice() is
	// called... it just doesn't do anything if there is no usb
	// device to work with.
	//
	// Timeout is in milliseconds.  Default is default USB timeout.
	void DoRead(int timeout = -1);

	// Utility function to make it easier for the user to create the
	// USB pure-read thread.  If the user wants anything more complicated
	// in this background thread, he can implement it himself and call
	// the above DoRead() in a loop.  If only the basics are needed,
	// then this makes it easy.
	// Throws Barry::ErrnoError on thread creation error.
	void SpinoffSimpleReadThread();
};


//
// DataHandle
//
/// std::auto_ptr like class that handles pointers to Data, but instead of
/// freeing them completely, the Data objects are turned to the
/// SocketRoutingQueue from whence they came.
///
class BXEXPORT DataHandle
{
private:
	SocketRoutingQueue &m_queue;
	mutable Data *m_data;

protected:
	void clear()
	{
		if( m_data ) {
			m_queue.ReturnBuffer(m_data);
			m_data = 0;
		}
	}

public:
	DataHandle(SocketRoutingQueue &q, Data *data)
		: m_queue(q)
		, m_data(data)
	{
	}

	DataHandle(const DataHandle &other)
		: m_queue(other.m_queue)
		, m_data(other.m_data)
	{
		// we now own the pointer
		other.m_data = 0;
	}

	~DataHandle()
	{
		clear();
	}

	Data* get()
	{
		return m_data;
	}

	Data* release()	// no longer owns the pointer, and does not free it
	{
		Data *ret = m_data;
		m_data = 0;
		return ret;
	}

	// frees current pointer, and takes ownership of new one
	void reset(Data *next = 0)
	{
		clear();
		m_data = next;
	}

	Data* operator->()
	{
		return m_data;
	}

	const Data* operator->() const
	{
		return m_data;
	}

	DataHandle& operator=(const DataHandle &other)
	{
		if( &m_queue != &other.m_queue )
			throw std::logic_error("Trying to copy DataHandles of different queues!");

		// remove our current data
		clear();

		// accept the new
		m_data = other.m_data;

		// we now own it
		other.m_data = 0;

		return *this;
	}

};


} // namespace Barry

#endif

