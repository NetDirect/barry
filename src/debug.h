
//#ifndef __DEBUG_H__
//#define __DEBUG_H__

#include <iostream>		// debugging only

#ifdef __DEBUG_MODE__
	#undef dout
	#undef eout
	#undef ddout

	// debug output
//	#define dout(x)  	std::cout << x << std::endl
	#define dout(x)

	// exception output
	#define eout(x)  	std::cout << x << std::endl

	// data dump output
	#define ddout(x)	std::cout << x << std::endl
#else
	#undef dout
	#undef eout
	#undef ddout

	#define dout(x)
	#define eout(x)  	std::cout << x << std::endl
	#define ddout(x)
#endif

//#endif

