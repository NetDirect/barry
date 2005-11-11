///
/// \file	error.cc
///		Common exception classes for the syncberry library
///

#include "error.h"
#include <sstream>

using namespace std;

namespace Syncberry {

std::string GetErrorString(int libusb_errno, const std::string &str)
{
	ostringstream oss;
	oss << "(" << libusb_errno << "): " << str;
	return oss.str();
}

SBError::SBError(int libusb_errno, const std::string &str)
	: std::runtime_error(GetErrorString(libusb_errno, str))
{
}

} // namespace Syncberry

