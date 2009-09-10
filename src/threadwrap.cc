///
/// \file	handler.cc
///		Thread handler class implementation
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/

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

