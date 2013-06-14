///
/// \file	ptyio.cc
///		Simple test program for pppob's -t pty option.
///

/*
    Copyright (C) 2012-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

struct ioconfig
{
	int read;
	int write;
};

void* readwrite(void *arg)
{
	struct ioconfig *io = (struct ioconfig *) arg;
	for (;;) {
		char buffer[4096];
		ssize_t count = read(io->read, buffer, sizeof(buffer));
		write(1, "Got data: ", 10);
		write(1, buffer, count);
		write(io->write, buffer, count);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if( argc < 2 )
		return 1;

	int slave = open(argv[1], O_RDWR);
	if( slave == -1 )
		return 2;

	// set raw mode
	struct termios tp;
	tcgetattr(slave, &tp);
	cfmakeraw(&tp);
	tcsetattr(slave, TCSANOW, &tp);

	struct ioconfig io1, io2;
	io1.read = 0;
	io1.write = slave;
	io2.read = slave;
	io2.write = 1;

	pthread_t iot1, iot2;
	pthread_create(&iot1, NULL, &readwrite, &io1);
	pthread_create(&iot2, NULL, &readwrite, &io2);

	pthread_join(iot1, NULL);
	pthread_join(iot2, NULL);

	return 0;
}

