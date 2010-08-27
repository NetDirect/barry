///
/// \file	m_raw_channel.h
///		Mode class for a raw channel
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)
    Portions Copyright (C) 2010 RealVNC Ltd.

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

#ifndef __BARRY_M_RAW_CHANNEL_H__
#define __BARRY_M_RAW_CHANNEL_H__

#include "dll.h"
#include "m_mode_base.h"
#include "socket.h"

#include <string>
#include <pthread.h>

namespace Barry {

class semaphore;

namespace Mode {

// Callback from the raw channel.

class BXEXPORT RawChannelDataCallback
{
public:
	// Called when data has been received on the channel
	virtual void DataReceived(Data& data) = 0;
	// Called when the channel has an error
	virtual void ChannelError(std::string msg) = 0;
	// Called when the channel has been asked to close by the other side
	virtual void ChannelClose() = 0;
};

//
// Raw channel class
//
/// The main class for creating a raw channel session.
///
/// To use this class, use the following steps:
///
/// - Implement RawChannelDataCallback
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::RawChannel object, passing in the Controller
///		object during construction
///	- Call Open() to open the channel and finish constructing.
///	- Call GetData() to fetch data
///	- Call SendData() to send data
///
class BXEXPORT RawChannel : public Mode
{
	// Mutex for signalling between read and write threads
	pthread_mutex_t m_mutex;
	bool m_mutex_valid;
	// Condvar for signalling between read and write threads
	pthread_cond_t m_cv;
	bool m_cv_valid;
	
	semaphore* m_semaphore;
	RawChannelDataCallback* m_callback;
	unsigned char *m_send_buffer;
	bool m_zero_registered;
	std::string* m_pending_error;

protected:
	void InitBuffer();
	void InitSemaphore();
	void SetPendingError(const char* msg);
	void UnregisterZeroSocketInterest();

public:
	// Creates a raw channel in non-callback mode. 
	// This requires all data to be sent and received
	// via calls to Send and Receive.
	// As there are no notifications of data being
	// available to send or receive, this is only recommended
	// for use with synchronous protocols over the channel.
	RawChannel(Controller& con);

	// Creates a raw channel in callback mode.
	// This requires all data to be sent via calls to Send, but
	// the Receive method must never be called.
	// Instead the DataReceive
	//
	// If using a raw channel in callback mode it's important
	// to regularily call DoRead on the SocketRoutingQueue
	// registered with the controller in a separate thread from
	// the thread(s) which call Send. Calls to Send will generally
	// block until a few calls to DoRead on the router are performed.
	// This will happen correctly if a read thread is created
	// with SocketRoutingQueue::SpinoffSimpleReadThread.
	RawChannel(Controller& con, RawChannelDataCallback& callback);

	virtual ~RawChannel();

	//////////////////////////////////
	// Raw channel mode specific methods

	// Send some data on the raw channel.
	// Will throw a Barry::Error if data is longer than
	// MaximumPacketContentsSize or a Barry::Usb::Error if there
	// is an underlying USB error.
	//
	// If using a raw channel in callback mode then care must be
	// taken to ensure another thread is running during any calls
	// to Send. See the comment in the constructor of RawChannel
	// for further information.
	void Send(Data& data, int timeout = -1);

	// Receive some data on the raw channel.
	// Will throw a Barry::Error if a disconnect occurs
	// or a Barry::Usb::Error if there is an underlying USB error
	// or a Barry::Usb::Timeout if the receive times out.
	//
	// The timeout value doesn't directly correspond to the maximum
	// time spent in this method. The timeout value specifies how
	// long to wait between any data arriving on the socket, not just 
	// for the RawChannel. This means that if there is USB data being
	// received over the port every second, but not for this channel,
	// that a timeout of two seconds would cause this method to never
	// return.
	//
	// If finer control of timeout between packets is required then it's
	// recommended to use the callback enabled version of the raw channel
	// and implement a timeout timer in the application using the
	// raw channel.
	//
	// Only valid to call this if the raw channel was created in non-callback
	// mode. If this is called when the raw channel was created with a
	// callback then a std::logic_error will be thrown.
	DataHandle Receive(int timeout = -1);

	// Returns the maximum quantity of data which
	// can be sent
	size_t MaximumSendSize();

	// Not intended for use by users of this class.
	// Instead data received will come in via the 
	// RawChannelDataCallback::DataReceived callback.
	void HandleReceivedData(Data& data);

	// Not intended for use by users of this class.
	// This method is called by the internals of
	// Barry when setting up a connection.
	void OnOpen();
};

}} // namespace Barry::Mode

#endif

