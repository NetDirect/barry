#ifndef __BARRYJDWP_DATA_H__
#define __BARRYJDWP_DATA_H__


namespace JDWP {


void AddDataByte(Barry::Data &data, size_t &size, const uint8_t value);
void AddDataInt(Barry::Data &data, size_t &size, const uint32_t value);
void AddDataChar(Barry::Data &data, size_t &size, const void *buf, size_t bufsize);
void AddDataString(Barry::Data &data, size_t &size, const std::string &str);

}

#endif

