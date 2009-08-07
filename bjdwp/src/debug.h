#ifndef __BARRYJDWP_DEBUG_H__
#define __BARRYJDWP_DEBUG_H__


#define jdwplog(x)	if(Barry::LogVerbose()) { Barry::LogLock lock; (*Barry::GetLogStream()) << x << std::endl; }

#endif

