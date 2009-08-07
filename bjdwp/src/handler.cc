#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <barry/endian.h>

#include "handler.h"


namespace JDWP {

JDWHandler::JDWHandler(int socket, void *(*callback)(void *data), void *data) {
	pthread_create(&thread, NULL, callback, data);
}


JDWHandler::~JDWHandler() {
	pthread_join(thread, NULL);
}


void JDWHandler::dispose() {
	pthread_cancel(thread);
}

} // namespace JDWP

