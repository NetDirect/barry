#ifndef __BARRYJDWP_MANAGER_H__
#define __BARRYJDWP_MANAGER_H__

#include <map>

#include <barry/jdwp/codinfo.h>


namespace JDWP {

class JDWAppList;
class JDWAppInfo;


class JDWAppList : public std::map<uint32_t, JDWAppInfo> {
protected:

public:

private:
};


class JDWAppInfo {
protected:

public:
	uint32_t uniqueId;
	std::string appName;

	Barry::JDG::JDGClassList classList;

	void load(Barry::JDG::JDGCodInfo &info);

private:
};


} // namespace JDWP


#endif

