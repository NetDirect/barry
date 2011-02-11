///
/// \file	ContactPhotoWidget.h
///		Bitmap button that shows a Contact::Image photo
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_CONTACT_PHOTO_WIDGET_H__
#define __BARRYDESKTOP_CONTACT_PHOTO_WIDGET_H__

#include <wx/wx.h>
#include <memory>

namespace Barry {
	class Contact;
}

class ContactPhotoWidget : public wxBitmapButton
{
	// external data
	Barry::Contact &m_rec;
	std::auto_ptr<wxBitmap> m_bitmap;

public:
	ContactPhotoWidget(wxWindow *parent, wxWindowID id,
		Barry::Contact &rec);

	static void DrawNoPhoto(wxBitmap &bm, int width, int height);
};

#endif

