///
/// \file	common.h
///		General header for the Barry library
///

#ifndef __BARRY_COMMON_H__
#define __BARRY_COMMON_H__

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001

#define BLACKBERRY_INTERFACE		0
#define BLACKBERRY_CONFIGURATION	1

#define WRITE_ENDPOINT		0x05
#define READ_ENDPOINT		0x82

namespace Barry {

void Init();

} // namespace Barry

#endif

