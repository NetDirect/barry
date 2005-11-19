///
/// \file	error.h
///		Common exception classes for the Barry library
///

#ifndef __BARRY_ERROR_H__
#define __BARRY_ERROR_H__

#include <stdexcept>

namespace Barry {

class SBError : public std::runtime_error
{
public:
	SBError(const std::string &str) : std::runtime_error(str) {}
	SBError(int libusb_errno, const std::string &str);
};

} // namespace Barry

#endif

