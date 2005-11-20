#ifndef __BARRY_BASE64_H__
#define __BARRY_BASE64_H__

#include <string>

// in-memory encode / decode
bool base64_encode(const std::string &in, std::string &out);
bool base64_decode(const std::string &in, std::string &out);

#endif

