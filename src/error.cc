///
/// \file	error.cc
///		Common exception classes for the Barry library
///

#include "error.h"
#include <sstream>
#include <iomanip>

using namespace std;

namespace Barry {

std::string GetErrorString(int libusb_errno, const std::string &str)
{
	ostringstream oss;
	oss << "(" << setbase(10) << libusb_errno << ", "
	    << strerror(-libusb_errno) << "): "
	    << str;
	return oss.str();
}

SBError::SBError(int libusb_errno, const std::string &str)
	: std::runtime_error(GetErrorString(libusb_errno, str))
{
}

} // namespace Barry

