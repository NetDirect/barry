///
/// \file	ContactPhotoWidget.cc
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

#include "ContactPhotoWidget.h"
#include "windowids.h"
#include <barry/barry.h>
#include <wx/mstream.h>


ContactPhotoWidget::ContactPhotoWidget(wxWindow *parent,
					wxWindowID id,
					Barry::Contact &rec)
	: m_rec(rec)
{
	// limit size of image to 60 px height
	int max_height = 60, width = 0;

	if( m_rec.Image.size() ) {
		// load m_rec.Image into a wxBitmap
		wxMemoryInputStream stream(m_rec.Image.data(),
			m_rec.Image.size());
		wxImage jpeg(stream, wxBITMAP_TYPE_JPEG);

		float ratio = (float)max_height / jpeg.GetHeight();
		width = jpeg.GetWidth() * ratio;

		jpeg.Rescale(width, max_height, wxIMAGE_QUALITY_HIGH);
		m_bitmap.reset( new wxBitmap(jpeg) );
	}

	// anything loaded?  if not, load "empty" bitmap
	if( !m_bitmap.get() ) {
		width = 50;
		max_height = 60;
		m_bitmap.reset( new wxBitmap(width, max_height) );
		DrawNoPhoto(*m_bitmap, width, max_height);
	}

	// have bitmap, create our bitmap button
	Create(parent, id, *m_bitmap, wxDefaultPosition,
		wxSize(width, max_height));
}

void ContactPhotoWidget::DrawNoPhoto(wxBitmap &bm, int width, int height)
{
	wxMemoryDC dc;
	dc.SelectObject(bm);

	// resources
	wxColour textcolour(0xa9, 0xa5, 0xa2);
	wxColour background(0xed, 0xec, 0xeb);
	wxPen pen(background);
	wxBrush brush(background);
	wxString line1(_T("No")), line2(_T("Photo"));
	int pointsize = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
	wxFont font(pointsize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
		wxFONTWEIGHT_NORMAL);

	// trim fontsize so it fits
	wxSize line1_extent, line2_extent;
	do {
		font.SetPointSize(pointsize--);
		dc.SetFont(font);
		line1_extent = dc.GetTextExtent(line1);
		line2_extent = dc.GetTextExtent(line2);
	} while( line1_extent.GetWidth() > width || line2_extent.GetWidth() > width);

	// setup DC
	dc.SetPen(pen);
	dc.SetBrush(brush);
	dc.SetTextForeground(textcolour);
	dc.SetTextBackground(background);

	// calculate position
	int total_height = line1_extent.GetHeight() + line2_extent.GetHeight();
	int y1_start = (height - total_height) / 2;
	int y2_start = y1_start + line1_extent.GetHeight();
	int x1_start = (width - line1_extent.GetWidth()) / 2;
	int x2_start = (width - line2_extent.GetWidth()) / 2;

	// draw
	dc.DrawRectangle(0, 0, width, height);
	dc.DrawText(line1, x1_start, y1_start);
	dc.DrawText(line2, x2_start, y2_start);

	// cleanup
	dc.SetPen(wxNullPen);
	dc.SetBrush(wxNullBrush);
}

