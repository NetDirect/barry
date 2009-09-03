#ifndef __BARRYJDWP_SERVER_H__
#define __BARRYJDWP_SERVER_H__

#include <barry/jdwp/codinfo.h>

#include "handler.h"
#include "manager.h"


namespace JDWP {

class JDWServer {
protected:

public:
	JDWServer(const char *address, int port);
	~JDWServer();

	void setDevice(Barry::Mode::JVMDebug *device);
	void setPasswordDevice(std::string password);

	void setConsoleCallback(void (*callback)(std::string message));

	bool start();
	void acceptConnection();
	void attachToDevice();
	void initVisibleClassList();
	bool hello();
	void run();
	void detachFromDevice();
	bool stop();

private:
	int fd;
	int sockfd;

	char *address;
	int port;

	bool loop;
	bool targetrunning;

	std::string password;
	Barry::Mode::JVMDebug *jvmdebug;

	Barry::JVMModulesList modulesList;				// List of COD applications installed on the device
	Barry::JDG::JDGDebugFileList debugFileList;		// List of debug file on the host

	JDWAppList appList;								// List of BlackBerry application (an application contents several COD files)
	Barry::JDG::JDGClassList visibleClassList;		// Visible class list from JDB

	JDWHandler *handler;
	void (*printConsoleMessage)(std::string message);

	void CommandsetProcess(Barry::Data &cmd);

	void CommandsetVirtualMachineProcess(Barry::Data &cmd);
	void CommandsetEventRequestProcess(Barry::Data &cmd);

	void CommandVersion(Barry::Data &cmd);
	void CommandIdSizes(Barry::Data &cmd);
	void CommandAllClasses(Barry::Data &cmd);
	void CommandAllThreads(Barry::Data &cmd);
	void CommandSuspend(Barry::Data &cmd);
	void CommandResume(Barry::Data &cmd);
	void CommandClassPaths(Barry::Data &cmd);

	void CommandSet(Barry::Data &cmd);

//	void BackgroundDeviceProcess();
};

} // namespace JDWP

#endif

