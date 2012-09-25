// Found at:
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// Note that these functions trim the same arguments passed in, and do not
// make copies.

#ifndef __BARRY_TRIM_H__
#define __BARRY_TRIM_H__

#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <locale>
#include <cctype>

namespace Barry { namespace Inplace {

// Windows CE defines std::isspace(int) as a macro to a function with
// two arguments with one prefilled, so a wrapper function is needed.
static inline int isSpaceChar(int c) {
	return std::isspace(c);
}

// trim from start
static inline std::string &ltrim(std::string &s) {
	std::string::iterator end(std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(isSpaceChar))));
	s.erase(s.begin(), end);
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
	std::string::reverse_iterator start(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(isSpaceChar))));
	s.erase(start.base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}

}} // namespace Barry::Inplace

#endif

