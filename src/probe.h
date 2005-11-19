///
/// \file	probe.h
///		USB Blackberry detection routines
///

#ifndef __BARRY_PROBE_H__
#define __BARRY_PROBE_H__

#include <libusb.h>
#include <vector>
#include <iosfwd>

// forward declarations
class Data;


namespace Barry {

struct ProbeResult
{
	libusb_device_id_t m_dev;
	uint32_t m_pin;
};

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr);


class Probe
{
	std::vector<ProbeResult> m_results;

	void Parse(const Data &data, ProbeResult &result);
public:
	Probe();

	int GetCount() const { return m_results.size(); }
	const ProbeResult& Get(int index) const { return m_results[index]; }
};


} // namespace Barry

#endif

