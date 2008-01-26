///
/// \file	router.h
///		Support classes for the pluggable socket routing system.
///

/*
    Copyright (C) 2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "router.h"
#include "scoped_lock.h"

///////////////////////////////////////////////////////////////////////////////
// SocketRoutingQueue constructors

SocketRoutingQueue::SocketRoutingQueue()
	: m_dev(0)
	, m_writeEp(0)
	, m_readEp(0)
	, m_interest(false)
	, m_mutex(PTHREAD_MUTEX_INITIALIZER)
	, m_continue_reading(false)
{
}

SocketRoutingQueue::~SocketRoutingQueue()
{
	// thread running?
	if( m_continue_reading ) {
		m_continue_reading = false;
		pthread_join(m_usb_read_thread, NULL);
	}
}

///////////////////////////////////////////////////////////////////////////////
// protected members

//
// SimpleReadThread()
//
/// Convenience thread to handle USB read activity.
///
void *SocketRoutingQueue::SimpleReadThread(void *userptr)
{
	SocketRoutingQueue *q = (SocketRoutingQueue *)userptr;

	// read from USB and write to stdout until finished
	while( q->m_continue_reading ) {
		q->DoRead(2000);	// timeout in milliseconds
	}
}


///////////////////////////////////////////////////////////////////////////////
// public API

// These functions connect the router to an external Usb::Device
// object.  Normally this is handled automatically by the
// Controller class, but are public here in case they are needed.
void SocketRoutingQueue::SetUsbDevice(Usb::Device *dev, int writeEp, int readEp)
{
	scoped_lock lock(&m_mutex);
	m_dev = dev;
	m_writeEp = writeEp;
	m_readEp = readEp;
}

void SocketRoutingQueue::ClearUsbDevice()
{
	scoped_lock lock(&m_mutex);
	m_dev = 0;
}

bool SocketRoutingQueue::UsbDeviceReady()
{
	scoped_lock lock(&m_mutex);
	return m_dev != 0;
}

// Called by the application's "read thread" to read the next usb
// packet and route it to the correct queue.  Returns after every
// read, even if a handler is associated with a queue.
// Note: this function is safe to call before SetUsbDevice() is
// called... it just doesn't do anything if there is no usb
// device to work with.
//
// Timeout is in milliseconds.
//
void SocketRoutingQueue::DoRead(int timeout = -1)
{
	scoped_lock lock(&m_mutex);

	// if we are not connected to a USB device yet, just wait
	if( !m_dev ) {
		lock.unlock();	// unlock early, since we're sleeping
		sleep(timeout / 1000 + 1);
		return;
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
		throw Barry::ErrnoError("SocketRoutingQueue: Error creating USB read thread.", ret);
	}
}

