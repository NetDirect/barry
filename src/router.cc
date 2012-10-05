///
/// \file	router.cc
///		Support classes for the pluggable socket routing system.
///

/*
    Copyright (C) 2008-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "router.h"
#include "scoped_lock.h"
#include "data.h"
#include "protostructs.h"
#include "protocol.h"
#include "usbwrap.h"
#include "endian.h"
#include "debug.h"
#include <unistd.h>
#include <iostream>
#include <iomanip>

using namespace std;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// SocketDataHandler default methods

void SocketRoutingQueue::SocketDataHandler::Error(Barry::Error &error)
{
	// Just log the error
	eout("SocketDataHandler: Error: " << error.what());
	(void) error;
}

SocketRoutingQueue::SocketDataHandler::~SocketDataHandler()
{
	// Nothing to destroy
}

///////////////////////////////////////////////////////////////////////////////
// SocketRoutingQueue constructors

SocketRoutingQueue::SocketRoutingQueue(int prealloc_buffer_count,
					int default_read_timeout)
	: m_dev(0)
	, m_writeEp(0)
	, m_readEp(0)
	, m_interest(false)
	, m_seen_usb_error(false)
	, m_timeout(default_read_timeout)
	, m_continue_reading(false)
{
	pthread_mutex_init(&m_mutex, NULL);

	pthread_mutex_init(&m_readwaitMutex, NULL);
	pthread_cond_init(&m_readwaitCond, NULL);

	AllocateBuffers(prealloc_buffer_count);
}

SocketRoutingQueue::~SocketRoutingQueue()
{
	// thread running?
	if( m_continue_reading ) {
		m_continue_reading = false;
		pthread_join(m_usb_read_thread, NULL);
	}

	// dump all unused packets to debug output
	SocketQueueMap::const_iterator b = m_socketQueues.begin();
	for( ; b != m_socketQueues.end(); ++b ) {
		DumpSocketQueue(b->first, b->second->m_queue);
	}
	if( m_default.size() ) {
		ddout("(Default queue is socket 0)");
		DumpSocketQueue(0, m_default);
	}
}

///////////////////////////////////////////////////////////////////////////////
// protected members

//
// ReturnBuffer
//
/// Provides a method of returning a buffer to the free queue
/// after processing.  The DataHandle class calls this automatically
/// from its destructor.
void SocketRoutingQueue::ReturnBuffer(Data *buf)
{
	// don't need to lock here, since m_free handles its own locking
	m_free.push(buf);
}

//
// QueuePacket
//
/// Helper function to add a buffer to a socket queue.
/// Returns false if no queue is available for that socket.
//// Also empties the DataHandle on success.
///
bool SocketRoutingQueue::QueuePacket(SocketId socket, DataHandle &buf)
{
	if( m_interest ) {
		bool seq = Protocol::IsSequencePacket(*buf.get());

		// lock so we can access the m_socketQueues map safely
		scoped_lock lock(m_mutex);

		// search for registration of socket
		SocketQueueMap::iterator qi = m_socketQueues.find(socket);
		if( qi != m_socketQueues.end() ) {
			if( seq ) {
				if( (qi->second->m_type & SequencePackets) == 0 )
					return false;
			}
			else {
				if( (qi->second->m_type & DataPackets) == 0 )
					return false;
			}
			qi->second->m_queue.push(buf.release());
			return true;
		}
	}

	return false;
}

bool SocketRoutingQueue::QueuePacket(DataQueue &queue, DataHandle &buf)
{
	// don't need to lock here, since queue handles its own locking
	queue.push(buf.release());
	return true;
}

//
// RouteOrQueuePacket
//
/// Same as QueuePacket, except sends the data to the callback if
/// a callback is available.
///
/// This function duplicates code from QueuePacket(), in order to
/// optimize the mutex locking.
///
bool SocketRoutingQueue::RouteOrQueuePacket(SocketId socket, DataHandle &buf)
{
	// search for registration of socket
	if( m_interest ) {
		// lock so we can access the m_socketQueues map safely
		scoped_lock lock(m_mutex);

		SocketQueueMap::iterator qi = m_socketQueues.find(socket);
		if( qi != m_socketQueues.end() ) {
			SocketDataHandlerPtr &sdh = qi->second->m_handler;

			// is there a handler?
			if( sdh ) {
				// unlock & let the handler process it
				lock.unlock();
				sdh->DataReceived(*buf.get());

				// no exceptions thrown, clear the
				// DataHandle, sending packet back to its
				// free list
				buf.reset();
				return true;
			}
			else {
				qi->second->m_queue.push(buf.release());
				return true;
			}
		}
	}

	return false;
}

//
// SimpleReadThread()
//
/// Convenience thread to handle USB read activity.
///
void *SocketRoutingQueue::SimpleReadThread(void *userptr)
{
	SocketRoutingQueue *q = (SocketRoutingQueue *)userptr;

	// read from USB and write to stdout until finished
	q->m_seen_usb_error = false;
	while( q->m_continue_reading ) {
		try {
			q->DoRead(1000);	// timeout in milliseconds
		}
		catch (std::runtime_error const &e) {
			eout(_("SimpleReadThread received uncaught exception: ") <<  typeid(e).name() << _(" what: ") << e.what());
		}
		catch (...) {
			eout(_("SimpleReadThread recevied uncaught exception of unknown type"));
		}
	}
	return 0;
}

void SocketRoutingQueue::DumpSocketQueue(SocketId socket, const DataQueue &dq)
{
	// dump a record of any unused packets in the queue, for debugging
	if( dq.size() ) {
		ddout(_("SocketRoutingQueue Leftovers: ")
			<< dec << dq.size()
			<< _(" packet(s) for socket: ") << "0x"
			<< hex << (unsigned int) socket
			<< "\n"
			<< dq);
	}
}


///////////////////////////////////////////////////////////////////////////////
// public API

// These functions connect the router to an external Usb::Device
// object.  Normally this is handled automatically by the
// Controller class, but are public here in case they are needed.
void SocketRoutingQueue::SetUsbDevice(Usb::Device *dev, int writeEp, int readEp,
					SocketDataHandlerPtr callback)
{
	scoped_lock lock(m_mutex);
	m_dev = dev;
	m_usb_error_dev_callback = callback;
	m_writeEp = writeEp;
	m_readEp = readEp;
}

void SocketRoutingQueue::ClearUsbDevice()
{
	scoped_lock lock(m_mutex);
	m_dev = 0;
	m_usb_error_dev_callback.reset();
	lock.unlock();

	// wait for the DoRead cycle to finish, so the external
	// Usb::Device object doesn't close before we're done with it
	scoped_lock wait(m_readwaitMutex);
	pthread_cond_wait(&m_readwaitCond, &m_readwaitMutex);
}

bool SocketRoutingQueue::UsbDeviceReady()
{
	scoped_lock lock(m_mutex);
	return m_dev != 0 && !m_seen_usb_error;
}

//
// AllocateBuffers
//
/// This class starts out with no buffers, and will grow one buffer
/// at a time if needed.  Call this to allocate count buffers
/// all at once and place them on the free queue.  After calling
/// this function, at least count buffers will exist in the free
/// queue.  If there are already count buffers, none will be added.
///
void SocketRoutingQueue::AllocateBuffers(int count)
{
	int todo = count - m_free.size();

	for( int i = 0; i < todo; i++ ) {
		// m_free handles its own locking
		m_free.push( new Data );
	}
}

//
// DefaultRead (both variations)
//
/// Returns the data for the next unregistered socket.
/// Blocks until timeout or data is available.
/// Returns false (or null pointer) on timeout and no data.
/// With the return version of the function, there is no
/// copying performed.
///
/// This version performs a copy.
///
bool SocketRoutingQueue::DefaultRead(Data &receive, int timeout)
{
	DataHandle buf = DefaultRead(timeout);
	if( !buf.get() )
		return false;

	// copy to desired buffer
	receive = *buf.get();
	return true;
}

///
/// This version does not perform a copy.
///
DataHandle SocketRoutingQueue::DefaultRead(int timeout)
{
	if( m_seen_usb_error && timeout == -1 ) {
		// If an error has been seen and not cleared then no
		// more data will be read into the queue by
		// DoRead(). Forcing the timeout to zero allows any
		// data already in the queue to be read, but prevents
		// waiting for data which will never arrive.
		timeout = 0;
	}

	// m_default handles its own locking
	// Be careful with the queue timeout, since its -1 means "forever"
	Data *buf = m_default.wait_pop(timeout == -1 ? m_timeout : timeout);
	return DataHandle(*this, buf);
}

//
// RegisterInterestAndType
//
/// Register an interest in data from a certain socket.  To read
/// from that socket, use the SocketRead() function from then on.
///
/// Any non-registered socket goes in the default queue
/// and must be read by DefaultRead()
///
/// If not null, handler is called when new data is read.  It will
/// be called in the same thread instance that DoRead() is called from.
/// Handler is passed the DataQueue Data pointer, and so no
/// copying is done.  Once the handler returns, the data is
/// considered processed and not added to the interested queue,
/// but instead returned to m_free.
///
/// Throws std::logic_error if already registered.
///
void SocketRoutingQueue::RegisterInterestAndType(SocketId socket,
						 SocketDataHandlerPtr handler,
						 InterestType type)
{
	// modifying our own std::map, need a lock
	scoped_lock lock(m_mutex);

	SocketQueueMap::iterator qi = m_socketQueues.find(socket);
	if( qi != m_socketQueues.end() )
		throw std::logic_error(_("RegisterInterest requesting a previously registered socket."));

	m_socketQueues[socket] = QueueEntryPtr( new QueueEntry(handler, type) );
	m_interest = true;
}

//
// RegisterInterest
//
/// This behaves like RegisterInterest(SocketId, SocketDataHandlerPtr, InterestType)
/// but defaults to an InterestType of SequenceAndDataPackets
///
void SocketRoutingQueue::RegisterInterest(SocketId socket,
					  SocketDataHandlerPtr handler)
{
	RegisterInterestAndType(socket, handler, SequenceAndDataPackets);
}

//
// UnregisterInterest
//
/// Unregisters interest in data from the given socket, and discards
/// any existing data in its interest queue.  Any new incoming data
/// for this socket will be placed in the default queue.
///
void SocketRoutingQueue::UnregisterInterest(SocketId socket)
{
	// modifying our own std::map, need a lock
	scoped_lock lock(m_mutex);

	SocketQueueMap::iterator qi = m_socketQueues.find(socket);
	if( qi == m_socketQueues.end() )
		return;	// nothing registered, done

	// dump a record of any unused packets in the queue, for debugging
	DumpSocketQueue(qi->first, qi->second->m_queue);

	// salvage all our data buffers
	m_free.append_from( qi->second->m_queue );

	// remove the QueueEntryPtr from the map
	m_socketQueues.erase( qi );

	// check the interest flag
	m_interest = m_socketQueues.size() > 0;
}

//
//
// ChangeInterest
//
/// Changes the type of data that a client is interested in for a certain socket.
/// Interest in the socket must have previously been registered by a call
/// to RegisterInterest().
void SocketRoutingQueue::ChangeInterest(SocketId socket, InterestType type)
{
	// modifying our own std::map, need a lock
	scoped_lock lock(m_mutex);

	SocketQueueMap::iterator qi = m_socketQueues.find(socket);
	if( qi == m_socketQueues.end() )
		throw std::logic_error("ChangeInterest requires a previously registered socket.");

	qi->second->m_type = type;
}

//
// SocketRead
//
/// Reads data from the interested socket cache.  Can only read
/// from sockets that have been previously registered.
///
/// Blocks until timeout or data is available.
///
/// Returns false (or null pointer) on timeout and no data.
/// With the return version of the function, there is no
/// copying performed.
///
/// Throws std::logic_error if a socket was requested that was
/// not previously registered.
///
/// Copying is performed with this function.
///
bool SocketRoutingQueue::SocketRead(SocketId socket, Data &receive, int timeout)
{
	DataHandle buf = SocketRead(socket, timeout);
	if( !buf.get() )
		return false;

	// copy to desired buffer
	receive = *buf.get();
	return true;
}

///
/// Copying is not performed with this function.
///
/// Throws std::logic_error if a socket was requested that was
/// not previously registered.
///
DataHandle SocketRoutingQueue::SocketRead(SocketId socket, int timeout)
{
	QueueEntryPtr qep;
	DataQueue *dq = 0;

	// accessing our own std::map, need a lock
	{
		scoped_lock lock(m_mutex);
		SocketQueueMap::iterator qi = m_socketQueues.find(socket);
		if( qi == m_socketQueues.end() )
			throw std::logic_error(_("SocketRead requested data from unregistered socket."));

		// got our queue, save the whole QueueEntryPtr (shared_ptr),
		// and unlock, since we will be waiting on the DataQueue,
		// not the socketQueues map
		//
		// This is safe, since even if UnregisterInterest is called,
		// our pointer won't be deleted until our shared_ptr
		// (QueueEntryPtr) goes out of scope.
		//
		// The remaining problem is that wait_pop() might wait
		// forever if there is no timeout... c'est la vie.
		// Should'a used a timeout. :-)
		qep = qi->second;
		dq = &qep->m_queue;
	}

	// get data from DataQueue
	// Be careful with the queue timeout, since its -1 means "forever"
	Data *buf = dq->wait_pop(timeout == -1 ? m_timeout : timeout);

	// specifically delete our copy of shared pointer, in a locked
	// environment
	{
		scoped_lock lock(m_mutex);
		qep.reset();
	}

	return DataHandle(*this, buf);
}

// Returns true if data is available for that socket.
bool SocketRoutingQueue::IsAvailable(SocketId socket) const
{
	scoped_lock lock(m_mutex);
	SocketQueueMap::const_iterator qi = m_socketQueues.find(socket);
	if( qi == m_socketQueues.end() )
		return false;
	return qi->second->m_queue.size() > 0;
}

//
// DoRead
//
/// Called by the application's "read thread" to read the next usb
/// packet and route it to the correct queue.  Returns after every
/// read, even if a handler is associated with a queue.
/// Note: this function is safe to call before SetUsbDevice() is
/// called... it just doesn't do anything if there is no usb
/// device to work with.
///
/// Timeout is in milliseconds.
//  This timeout is for the USB subsystem, so no special handling
//  for it is needed... just use usbwrap's default timeout.
void SocketRoutingQueue::DoRead(int timeout)
{
	class ReadWaitSignal
	{
		pthread_mutex_t &m_Mutex;
		pthread_cond_t &m_Cond;
	public:
		ReadWaitSignal(pthread_mutex_t &mut, pthread_cond_t &cond)
			: m_Mutex(mut), m_Cond(cond)
			{}
		~ReadWaitSignal()
		{
			scoped_lock wait(m_Mutex);
			pthread_cond_signal(&m_Cond);
		}
	} readwait(m_readwaitMutex, m_readwaitCond);

	Usb::Device * volatile dev = 0;
	int readEp;
	DataHandle buf(*this, 0);

	// if we are not connected to a USB device yet, just wait
	{
		scoped_lock lock(m_mutex);

		if( !m_dev || m_seen_usb_error ) {
			lock.unlock();	// unlock early, since we're sleeping
			// sleep only a short time, since things could be
			// in the process of setup or teardown
			usleep(125000);
			return;
		}

		dev = m_dev;
		readEp = m_readEp;

		// fetch a free buffer
		Data *raw = m_free.pop();
		if( !raw )
			buf = DataHandle(*this, new Data);
		else
			buf = DataHandle(*this, raw);
	}

	// take a chance and do the read unlocked, as this has the potential
	// for blocking for a while
	try {

		Data &data = *buf.get();

		if( !dev->BulkRead(readEp, data, timeout) )
			return;	// no data, done!

		MAKE_PACKET(pack, data);

		// make sure the size is right
		if( data.GetSize() < SB_PACKET_SOCKET_SIZE )
			return;	// bad size, just skip

		// extract the socket from the packet
		uint16_t socket = btohs(pack->socket);

		// if this is a sequence packet, handle it specially
		if( Protocol::IsSequencePacket(data) ) {
			// sequence.socket is a single byte
			socket = pack->u.sequence.socket;

			//////////////////////////////////////////////
			// ALWAYS queue sequence packets, so that
			// the socket code can handle SyncSend()
			if( !QueuePacket(socket, buf) ) {
				// if no queue available for this
				// socket, send it to the default
				// queue
				QueuePacket(m_default, buf);
			}

			// done with sequence packet
			return;
		}

		// we have data, now route or queue it
		if( RouteOrQueuePacket(socket, buf) )
			return; // done

		// if we get here, send to default queue
		QueuePacket(m_default, buf);
	}
	catch( Usb::Timeout & ) {
		// this is expected... just ignore
	}
	catch( Usb::Error &ue ) {
		// set the flag first, in case any of the handlers
		// are able to recover from this error
		m_seen_usb_error = true;

		// this is unexpected, but we're in a thread here...
		// Need to iterate through all the registered handlers
		// calling their error callback.
		// Can't be locked when calling the callback, so need
		// to make a list of them first.
		scoped_lock lock(m_mutex);
		std::vector<SocketDataHandlerPtr> handlers;
		SocketQueueMap::iterator qi = m_socketQueues.begin();
		while( qi != m_socketQueues.end() ) {
			SocketDataHandlerPtr &sdh = qi->second->m_handler;
			// is there a handler?
			if( sdh ) {
				handlers.push_back(sdh);
			}
			++qi;
		}

		SocketDataHandlerPtr usb_error_handler = m_usb_error_dev_callback;

		lock.unlock();
		std::vector<SocketDataHandlerPtr>::iterator hi = handlers.begin();
		while( hi != handlers.end() ) {
			(*hi)->Error(ue);
			++hi;
		}

		// and finally, call the specific error callback if available
		if( usb_error_handler.get() ) {
			usb_error_handler->Error(ue);
		}
	}
}

void SocketRoutingQueue::SpinoffSimpleReadThread()
{
	// signal that it's ok to run inside the thread
	if( m_continue_reading )
		return;	// already running
	m_continue_reading = true;

	// Start USB read thread, to handle all routing
	int ret = pthread_create(&m_usb_read_thread, NULL, &SimpleReadThread, this);
	if( ret ) {
		m_continue_reading = false;
		throw Barry::ErrnoError(_("SocketRoutingQueue: Error creating USB read thread."), ret);
	}
}

} // namespace Barry

