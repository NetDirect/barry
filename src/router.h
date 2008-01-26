///
/// \file	router.h
///		Support classes for the pluggable socket routing system.
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <stdint.h>
#include <map>
#include <tr1/memory>
#include <pthread.h>
#include "dataqueue.h"

namespace Usb { class Device; }

namespace Barry {

class DataHandle;

class SocketRoutingQueue
{
	friend class DataHandle;

public:
	typedef void (*SocketDataHandler)(Data*);	//< See RegisterInterest() for information on this callback.
	typedef std::pair<SocketDataHandler, DataQueue>	QueuePair;
	typedef std::tr1::shared_ptr<QueuePair>		QueuePairPtr;
	typedef std::map<uint16_t, QueuePairPtr>	SocketQueueMap;

private:
	Usb::Device *m_dev;
	int m_writeEp, m_readEp;

	bool m_interest;	// true if at least one socket has an interest.
				// used to optimize the reading

	pthread_mutex_t m_mutex;// controls access to local data, but not
				// DataQueues, as they have their own
				// locking per queue

	DataQueue m_free;
	DataQueue m_default;
	SocketQueueMap m_socketQueues;

protected:
	// Provides a method of returning a buffer to the free queue
	// after processing.  The DataHandle class calls this automatically
	// from its destructor.
	void ReturnBuffer(Data *buf);

	static void SimpleReadThread(void *userptr);

public:
//	SocketRoutingQueue(Usb::Device &dev, int writeEp, int readEp);
	SocketRoutingQueue();
	~SocketRoutingQueue();

	// These functions connect the router to an external Usb::Device
	// object.  Normally this is handled automatically by the
	// Controller class, but are public here in case they are needed.
	void SetUsbDevice(Usb::Device *dev);
	void ClearUsbDevice();

	// This class starts out with no buffers, and will grow one buffer
	// at a time if needed.  Call this to allocate count buffers
	// all at once and place them on the free queue.
	void AllocateBuffers(int count);

	// Returns the data for the next unregistered socket.
	// Blocks until timeout or data is available.
	// Returns false (or null pointer) on timeout and no data.
	// With the return version of the function, there is no
	// copying performed.
	bool DefaultRead(Data &receive, int timeout = -1);
	DataHandle DefaultRead(int timeout = -1);

	// Register an interest in data from a certain socket.  To read
	// from that socket, use the SocketRead() function from then on.
	// Any non-registered socket goes in the default queue
	// and must be read by DefaultRead()
	// If not null, handler is called when new data is read.  It will
	// be called in the same thread instance that DoRead() is called from.
	// Handler is passed the DataQueue Data pointer, and so no
	// copying is done.  Once the handler returns, the data is
	// considered processed and not added to the interested queue,
	// but instead returned to m_free.
	void RegisterInterest(uint16_t socket, SocketDataHandler handler = 0);

	// Unregisters interest in data from the given socket, and discards
	// any existing data in its interest queue.  Any new incoming data
	// for this socket will be placed in the default queue.
	void UnregisterInterest(uint16_t socket);

	// Reads data from the interested socket cache.  Can only read
	// from sockets that have been previously registered.
	// Blocks until timeout or data is available.
	// Returns false (or null pointer) on timeout and no data.
	// With the return version of the function, there is no
	// copying performed.
	bool SocketRead(uint16_t socket, Data &receive, int timeout = -1);
	DataHandle SocketRead(uint16_t socket, int timeout = -1);

	// Returns true if data is available for that socket.
	bool IsAvailable(uint16_t socket) const;

	// Called by the application's "read thread" to read the next usb
	// packet and route it to the correct queue.  Returns after every
	// read, even if a handler is associated with a queue.
	void DoRead(int timeout = -1);

	// Utility function to make it easier for the user to create the
	// USB pure-read thread.  If the user wants anything more complicated
	// in this background thread, he can implement it himself and call
	// the above DoRead() in a loop.  If only the basics are needed,
	// then this makes it easy.
	bool SpinoffSimpleReadThread();
};



class DataHandle
{
private:
	SocketRoutingQueue &m_queue;
	Data *m_data;

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
		m_data = 0;
	}

	~DataHandle()
	{
		if( m_data )
			m_queue.ReturnBuffer(m_data);
	}

	Data* operator->()
	{
		return m_data;
	}

	const Data* operator->() const
	{
		return m_data;
	}

	DataHandle& operator=(const DataHandle &other);
};


} // namespace Barry

#endif

