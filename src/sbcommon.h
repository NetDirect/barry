///
/// \file	sbcommon.h
///		General header for the syncberry library
///

#ifndef __SYNCBERRY_SBCOMMON_H__
#define __SYNCBERRY_SBCOMMON_H__

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001

#define BLACKBERRY_INTERFACE		0
#define BLACKBERRY_CONFIGURATION	1

#define WRITE_ENDPOINT		0x05
#define READ_ENDPOINT		0x82

namespace Syncberry {

void Init();

} // namespace Syncberry

#endif

