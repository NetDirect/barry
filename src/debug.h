
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>		// debugging only

#ifdef __DEBUG_MODE__
	#define dout(x)  std::cout << x << std::endl
#else
	#define dout(x)
#endif

#endif

