#ifndef __BARRYJDWP_JDWP_H__
#define __BARRYJDWP_JDWP_H__


#include <barry/barry.h>


namespace Barry { class Data; }


namespace JDWP {

/// \addtogroup exceptions
/// @{

/// Thrown on low level JDWP errors.
class Error : public Barry::Error
{
	int m_errcode;

public:
	Error(const std::string &str);
	Error(int errcode, const std::string &str);

	// can return 0 in some case, if unknown error code
	int errcode() const { return m_errcode; }
};

class Timeout : public Error
{
public:
	Timeout(const std::string &str) : Error(str) {}
	Timeout(int errcode, const std::string &str)
		: Error(errcode, str) {}
};

/// @}



class JDWP {
protected:

private:
	int m_lasterror;

public:
	JDWP();
	~JDWP();

	bool Read(int socket, Barry::Data &data, int timeout = -1);
	bool Write(int socket, const Barry::Data &data, int timeout = -1);
	bool Write(int socket, const void *data, size_t size, int timeout = -1);
};

} // namespace JDWP

#endif

