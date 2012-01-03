///
/// \file	main.cc
///		Entry point for barrybackup GUI
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

#include <iostream>
#include <stdexcept>
#include <errno.h>
#include <gtkmm.h>
#include <libglademm.h>
#include <barry/barry.h>
#include "BackupWindow.h"
#include "util.h"
#include "i18n.h"

//
// The catch-all handler for exceptions that occur inside signals
//
void main_exception_handler()
{
	try {
		throw;
	}
	catch( Glib::Exception &e ) {
		std::cerr << "Glib::Exception caught in main: " << std::endl;
		std::cerr << e.what() << std::endl;
		Gtk::MessageDialog msg(e.what());
		msg.run();
	}
	catch( Usb::Error &e ) {
		std::cerr << "Usb::Error caught in main:\n"
			<< e.what() << std::endl;

		// special check for EBUSY to make the error message
		// more user friendly
		Gtk::MessageDialog msg("");
		if( e.system_errcode() == -EBUSY ) {
			msg.set_message("Device busy.  This is likely due to the usb_storage kernel module being loaded.  Try 'rmmod usb_storage'.");
		}
		else {
			msg.set_message(e.what());
		}
		msg.run();
	}
	catch( std::exception &e ) {
		std::cerr << "std::exception caught in main: " << std::endl;
		std::cerr << e.what() << std::endl;
		Gtk::MessageDialog msg(e.what());
		msg.run();
	}
}


//
// These are for debugging... hopefully should never need them in the field,
// but you never know...
//

void (*old_unexpected_handler)() = 0;
void unexpected_handler()
{
	std::cerr << "main.cc: Unexpected exception detected - check your throw() specifications" << std::endl;
	(*old_unexpected_handler)();
}

void (*old_terminate_handler)() = 0;
void terminate_handler()
{
	std::cerr << "main.cc: terminate_handler() called - exception handling "
		"could not complete, most likely due to exception inside "
		"a destructor or catch()" << std::endl;
	(*old_terminate_handler)();
}



int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	old_unexpected_handler = std::set_unexpected(&unexpected_handler);
	old_terminate_handler = std::set_terminate(&terminate_handler);

	Glib::thread_init();

	using namespace Glib;
	OptionEntry debug_opt;
	debug_opt.set_flags(
		OptionEntry::FLAG_NO_ARG |
		OptionEntry::FLAG_IN_MAIN |
		OptionEntry::FLAG_OPTIONAL_ARG);
	debug_opt.set_short_name('d');
	debug_opt.set_long_name("debug-output");
	debug_opt.set_description("Enable protocol debug output to stdout/stderr");

	OptionGroup option_group("barry",
		"Options specific to the Barry Backup application.",
		"Options specific to the Barry Backup application.");
	bool debug_flag = false;
	option_group.add_entry(debug_opt, debug_flag);

	OptionContext option_context("Backup program for the Blackberry Handheld");
	option_context.add_group(option_group);

	try {

		Gtk::Main app(argc, argv, option_context);
		Glib::add_exception_handler( sigc::ptr_fun(main_exception_handler) );

		Barry::Init(debug_flag);

		Glib::RefPtr<Gnome::Glade::Xml> refXml = LoadXml("BackupWindow.glade");

		BackupWindow *pWnd = 0;
		refXml->get_widget_derived("BackupWindow", pWnd);
		std::auto_ptr<BackupWindow> apWnd(pWnd);

		Gtk::Main::run(*pWnd);

	}
	catch( Glib::OptionError &oe ) {
		// Ouch!  Not all glibmm versions provide an
		// easy way to display the official help... so we
		// at least point the user in the right direction.
		std::cerr << oe.what() << std::endl;
		std::cerr << "Try --help or --help-all" << std::endl;
		return 1;
	}
	catch(...) {
		main_exception_handler();
		return 1;
	}
}

