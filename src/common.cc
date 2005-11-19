///
/// \file	common.cc
///		General Barry interface routines
///

#include "common.h"
#include <libusb.h>

namespace Barry {

void Init()
{
	libusb_set_debug(9);
	libusb_init();
}

} // namespace Barry

