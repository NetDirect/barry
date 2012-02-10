///
/// \file	wxval.h
///		Homemade validators derived from wxValidator
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_WXVAL_H__
#define __BARRYDESKTOP_WXVAL_H__

#include <algorithm>

class DateTimeValidator : public wxValidator
{
	wxDateTime *m_pDateTime;
	mutable wxDatePickerCtrl *m_pCtrl;

public:
	explicit DateTimeValidator(wxDateTime *pval)
		: m_pDateTime(pval)
	{
	}

	DateTimeValidator(const DateTimeValidator &other)
		: m_pDateTime(other.m_pDateTime)
	{
	}

	virtual wxValidator* Clone() const
	{
		return new DateTimeValidator(*this);
	}

	bool CheckState() const
	{
		if( !m_pDateTime )
			return false;
		if( !m_validatorWindow )
			return false;

		if( !m_validatorWindow->IsKindOf(CLASSINFO(wxDatePickerCtrl)) )
			return false;

		m_pCtrl = dynamic_cast<wxDatePickerCtrl*>(m_validatorWindow);
		return m_pCtrl;
	}

	virtual bool Validate(wxWindow *parent)
	{
		// Validate() checks the *control's* value, not the
		// in-memory value, according to the wxWidgets documentation,
		// so do TransferFromWindow() first
		if( !(TransferFromWindow() && m_pDateTime->IsValid()) ) {
			wxMessageBox(_T("Invalid date!"), _T("Validation"),
				wxOK | wxICON_INFORMATION, parent);
			return false;
		}
		return true;
	}

	virtual bool TransferToWindow()
	{
		if( !CheckState() )
			return false;

		if( !m_pDateTime->IsValid() )
			return false;
		m_pCtrl->SetValue(*m_pDateTime);
		return true;
	}

	virtual bool TransferFromWindow()
	{
		if( !CheckState() )
			return false;

		*m_pDateTime = m_pCtrl->GetValue();
		return true;
	}
};

template <class EnumT>
class RadioBoxValidator : public wxValidator
{
	mutable std::vector<EnumT> m_codes;
	EnumT *m_pEnum;
	mutable wxRadioBox *m_pCtrl;

public:
	explicit RadioBoxValidator(EnumT *pval)
		: m_pEnum(pval)
	{
	}

	RadioBoxValidator(const RadioBoxValidator &other)
		: m_codes(other.m_codes)
		, m_pEnum(other.m_pEnum)
	{
	}

	virtual wxValidator* Clone() const
	{
		return new RadioBoxValidator(*this);
	}

	// return const reference to self to allow .Add().Add() series
	// on constructor
	const RadioBoxValidator& Add(EnumT code) const
	{
		m_codes.push_back(code);
		return *this;
	}

	bool CheckState() const
	{
		if( !m_pEnum )
			return false;
		if( !m_validatorWindow )
			return false;

		if( !m_validatorWindow->IsKindOf(CLASSINFO(wxRadioBox)) )
			return false;

		m_pCtrl = dynamic_cast<wxRadioBox*>(m_validatorWindow);
		return m_pCtrl;
	}

	bool IsSelectionValid() const
	{
		return std::find(m_codes.begin(), m_codes.end(), *m_pEnum)
			!= m_codes.end();
	}

	virtual bool Validate(wxWindow *parent)
	{
		// Validate() checks the *control's* value, not the
		// in-memory value, according to the wxWidgets documentation,
		// so do TransferFromWindow() first
		if( !(TransferFromWindow() && IsSelectionValid()) ) {
			wxMessageBox(
				_T("Please select one of the radio buttons."),
				_T("Validation"),
				wxOK | wxICON_INFORMATION, parent);
			return false;
		}
		return true;
	}

	virtual bool TransferToWindow()
	{
		if( !CheckState() )
			return false;

		typename std::vector<EnumT>::iterator i = std::find(
			m_codes.begin(), m_codes.end(), *m_pEnum);
		if( i == m_codes.end() )
			return false;
		m_pCtrl->SetSelection(i - m_codes.begin());
		return true;
	}

	virtual bool TransferFromWindow()
	{
		if( !CheckState() )
			return false;

		*m_pEnum = m_codes.at(m_pCtrl->GetSelection());
		return true;
	}
};

template <class EnumT>
RadioBoxValidator<EnumT> MakeRadioBoxValidator(EnumT *pval)
{
	return RadioBoxValidator<EnumT>(pval);
}

#endif

