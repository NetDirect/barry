///
/// \file	error.h
///		Common exception classes for the syncberry library
///

#ifndef __SYNCBERRY_ERROR_H__
#define __SYNCBERRY_ERROR_H__

#include <stdexcept>

namespace Syncberry {

class SBError : public std::runtime_error
{
public:
	SBError(const std::string &str) : std::runtime_error(str) {}
	SBError(int libusb_errno, const std::string &str);
};

} // namespace Syncberry

#endif

