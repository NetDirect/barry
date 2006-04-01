///
/// \file	barry.h
///		Main header file for applications
///

#ifndef __BARRY_BARRY_H__
#define __BARRY_BARRY_H__

/**

	\mainpage Barry Reference Manual


	\section getting_started Getting Started

	Welcome to the Barry reference manual.  This entire manual was
	generated via Doxygen from comments in the code.  You can view
	the code here as well, in the Files section.

	The best place to get started at the moment is to examine the
	source code to the Barry command line tool: btool.cc


	\section classes Major Classes

	To get started with the API, see the Barry::Controller class.

*/


// This lists all the headers that the application needs.
// Only these headers get installed.

#include "data.h"
#include "usbwrap.h"			// to be moved to libusb someday
#include "common.h"			// Init()
#include "error.h"			// exceptions
#include "probe.h"			// device prober class
#include "socket.h"
#include "protocol.h"			// application-safe header
#include "parser.h"
#include "builder.h"
#include "record.h"
#include "controller.h"
#include "controllertmpl.h"

#ifdef __BOOST_MODE__
// Boost serialization seems to be picky about header order, do them all here
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include "s11n-boost.h"
#endif

#endif

