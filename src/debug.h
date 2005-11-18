
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

	// easy exception output
	#define eeout(c, r)	std::cout << "Sent packet:\n" << c << "\n" << "Response packet:\n" << r << "\n"

	// data dump output
	#define ddout(x)	std::cout << x << std::endl
//	#define ddout(x)
#else
	#undef dout
	#undef eout
	#undef ddout

	#define dout(x)
	#define eout(x)  	std::cout << x << std::endl
	#define eeout(c, r)	std::cout << "Sent packet:\n" << c << "\n" << "Response packet:\n" << r << "\n"
	#define ddout(x)
#endif

//#endif

