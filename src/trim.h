// Found at:
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// Note that these functions trim the same arguments passed in, and do not
// make copies.

#ifndef __BARRY_TRIM_H__
#define __BARRY_TRIM_H__

#include <algorithm>
#include <functional>
#include <locale>

namespace Barry { namespace Inplace {

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}

}} // namespace Barry::Inplace

#endif

