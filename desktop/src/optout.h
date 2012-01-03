///
/// \file	OptOut.h
///		A set of container classes and element base class that support
///		smart elements that can remove themselves from the container
///		upon deletion.
///
///		To use it, derive your element from OptOut::Element, and
///		then add new-ly allocated pointers to the Vector.  The
///		Vector will delete any remaining elements, and each
///		element can be deleted early, either with a delete this
///		by themselves, or deleted manually outside of the Vector.
///

/*
    Copyright (C) 2010-2012, Chris Frey <cdfrey@foursquare.net>

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

#ifndef __UTIL_OPTOUT_H__
#define __UTIL_OPTOUT_H__

#include <vector>

namespace OptOut {

class ContainerBase;

class Element
{
	friend class ContainerBase;

	ContainerBase *m_container;

protected:
	virtual void OptIn(ContainerBase *container)
	{
		OptOut();
		m_container = container;
	}

	virtual void OptOut();

public:
	Element() : m_container(0) {}
	virtual ~Element()
	{
		OptOut();
	}
};

class ContainerBase
{
	friend class Element;

protected:
	virtual void NotifyElement(Element *element)
	{
		element->OptIn(this);
	}

	virtual void OptOut(Element *element) = 0;

public:
	virtual ~ContainerBase()
	{
	}
};

inline void Element::OptOut()
{
	if( m_container ) {
		m_container->OptOut(this);
		m_container = 0;
	}
}

template <class TypeT>
class Vector : public ContainerBase
{
public:
	typedef std::vector<Element*>			container_type;
	typedef container_type::iterator		iterator;
	typedef container_type::const_iterator		const_iterator;
	typedef container_type::size_type		size_type;

private:
	container_type m_con;

protected:
	virtual void OptOut(Element *element)
	{
		for( iterator i = m_con.begin(); i != m_con.end(); ++i ) {
			if( (*i) == element ) {
				m_con.erase(i);
				return;
			}
		}
	}

public:
	~Vector()
	{
		iterator i;
		while( (i = m_con.begin()) != m_con.end() ) {
			// this will remove the element from the array,
			// hence the while, starting over each time
			delete (*i);
		}
	}

	// std::vector<> compatible functions
	size_type size() { return m_con.size(); }
	void push_back(Element *element)
	{
		NotifyElement(element);
		m_con.push_back(element);
	}
	TypeT* operator[] (size_type n)
	{
		return dynamic_cast<TypeT*> (m_con[n]);
	}
	iterator begin() { return m_con.begin(); }
	const_iterator begin() const { return m_con.begin(); }
	iterator end() { return m_con.end(); }
	const_iterator end() const { return m_con.end(); }
	iterator rbegin() { return m_con.rbegin(); }
	const_iterator rbegin() const { return m_con.rbegin(); }
	iterator rend() { return m_con.rend(); }
	const_iterator rend() const { return m_con.rend(); }

	// since iterators hold Element* pointers, these functions
	// are just a helpful dynamic cast aid
	TypeT* GetType(iterator i) { return dynamic_cast<TypeT*> (*i); }
	const TypeT* GetType(const_iterator i) { return dynamic_cast<const TypeT*> (*i); }

};

} // namespace OptOut

#endif

