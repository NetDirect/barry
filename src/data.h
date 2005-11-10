///
/// \file	data.h
///		Class to deal with pre-saved data files
///

#ifndef __SB_DATA_H__
#define __SB_DATA_H__

#include <iosfwd>
#include <vector>

class Data
{
	unsigned char *m_data;
	int m_bufsize;			//< size of m_data buffer allocated
	int m_datasize;			//< number of bytes of actual data
	int m_endpoint;

	// output format flags
	static bool bPrintAscii;

protected:
	void MakeSpace(int desiredsize);

public:
	Data();
	Data(int endpoint, int startsize = 0x4000);
	Data(const Data &other);
	~Data();

	void InputHexLine(std::istream &is);
	void DumpHexLine(std::ostream &os, int index, int size) const;
	void DumpHex(std::ostream &os) const;

	int GetEndpoint() const { return m_endpoint; }

	const unsigned char * GetData() const { return m_data; }
	int GetSize() const { return m_datasize; }

	unsigned char * GetBuffer() { return m_data; }
	int GetBufSize() const { return m_bufsize; }
	void ReleaseBuffer(int datasize = -1);

	void Zap();

	Data& operator=(const Data &other);


	// static functions
	static void PrintAscii(bool setting) { bPrintAscii = setting; }
	static bool PrintAscii() { return bPrintAscii; }
};

std::istream& operator>> (std::istream &is, Data &data);
std::ostream& operator<< (std::ostream &os, const Data &data);


class Diff
{
	const Data &m_old, &m_new;

	void Compare(std::ostream &os, int index, int size) const;

public:
	Diff(const Data &old, const Data &new_);

	void Dump(std::ostream &os) const;
};

std::ostream& operator<< (std::ostream &os, const Diff &diff);


// utility functions
bool LoadDataArray(const std::string &filename, std::vector<Data> &array);

#endif

