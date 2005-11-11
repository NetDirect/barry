///
/// \file	syncberry.cc
///		General Blackberry interface routines
///

#include "sbcommon.h"
#include <libusb.h>

namespace Syncberry {

void Init()
{
	libusb_set_debug(9);
	libusb_init();
}

} // namespace Syncberry

