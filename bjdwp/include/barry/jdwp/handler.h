#ifndef __BARRYJDWP_HANDLER_H__
#define __BARRYJDWP_HANDLER_H__


namespace JDWP {

class JDWHandler {
protected:

public:
	JDWHandler(int socket, void *(*callback)(void *data), void *data);

	void dispose();

private:
	pthread_t thread;

	~JDWHandler();
};

} // namespace JDWP

#endif

