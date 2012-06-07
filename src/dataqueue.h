///
/// \file	dataqueue.h
///		FIFO queue of Data objects
///

/*
    Copyright (C) 2007-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_DATAQUEUE_H__
#define __BARRY_DATAQUEUE_H__

#include "dll.h"
#include <list>
#include <pthread.h>
#include <iosfwd>

namespace Barry {

class Data;

//
// DataQueue class
//
/// This class provides a thread aware fifo queue for Data objects,
/// providing memory management for all Data object pointers it contains.
///
/// It uses similar member names as std::queue<>, for consistency.
///
class BXEXPORT DataQueue
{
	// always use the raw_push() and raw_pop() functions
	typedef std::list<Data*>			queue_type;

	pthread_mutex_t m_waitMutex;
	pthread_cond_t m_waitCond;

	mutable pthread_mutex_t m_accessMutex;	// locked for each access of m_queue

	queue_type m_queue;

protected:
	void raw_push(Data *data);
	Data* raw_pop();

public:
	DataQueue();
	~DataQueue();		// frees all data in the queue

	// Pushes data into the end of the queue.
	// The queue owns this pointer as soon as the function is
	// called.  In the case of an exception, it will be freed.
	// Performs a thread broadcast once new data has been added.
	void push(Data *data);

	// Pops the next element off the front of the queue.
	// Returns 0 if empty.
	// The queue no longer owns this pointer upon return.
	Data* pop();

	// Pops the next element off the front of the queue, and
	// waits until one exists if empty.  If still no data
	// on timeout, returns null.
	// Timeout specified in milliseconds.  Default is wait forever.
	Data* wait_pop(int timeout = -1);

	// Pops all data from other and appends it to this.
	// After calling this function, other will be empty, and
	// this will contain all its data.
	// In the case of an exception, any uncopied data will
	// remain in other.
	void append_from(DataQueue &other);

	bool empty() const;	// return true if empty
	size_t size() const;

	void DumpAll(std::ostream &os) const;
};

inline std::ostream& operator<< (std::ostream &os, const DataQueue &dq)
{
	dq.DumpAll(os);
	return os;
}

} // namespace Barry

#endif

