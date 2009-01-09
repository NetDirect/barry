//
// ktrans.cc - print human readable version of usbfs_snoop kernel log
//

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <iomanip>
#include <sstream>
#include <string>
#include <string.h>
#include <barry/data.h>

using namespace std;

void Usage()
{
	cout << "ktrans - usbfs_snoop kernel log translator\n"
	        "\n"
		"\tUsage:   ktrans logmarker < log\n"
		"\n"
		"\tExample:  ktrans \"kernel: \" < /var/log/kern.log\n"
		"\n";
}

bool IsHexData(const char *logmarker, const char *str)
{
	str = strstr(str, logmarker);
	if( !str )
		return false;
	str += strlen(logmarker);

	if( strstr(str, "data ") || strstr(str, "data: ") ) {
		// looks like a data line, make sure data from there on out
		// is hex data

		// skip "data"
		str = strstr(str, "data") + 4;
		for( ; *str == ':' || *str == ' '; str++ )
			;

		while( *str ) {
			if( !( isdigit(*str) || *str == ' ' ||
			    (*str >= 'a' && *str <= 'f')) )
				return false;
			str++;
		}

		return true;
	}
	else {
		while( *str ) {
			if( !( isdigit(*str) || *str == ' ' ||
			    (*str >= 'a' && *str <= 'f')) )
				return false;
			str++;
		}
		return true;	// all data is numeric, so this is a continuation line
	}
}

void SplitHex(const char *logmarker, const char *str, Barry::Data &data)
{
	const char *hexptr = strstr(str, logmarker);
	if( !hexptr ) {
		cout << str << endl;
		return;
	}
	hexptr += strlen(logmarker);
	std::string readable(str, hexptr - str);

	str = hexptr;

	hexptr = strstr(str, "data ");
	if( hexptr ) {
		hexptr += 5;
	}
	else {
		hexptr = strstr(str, "data: ");
		if( hexptr )
			hexptr += 6;
	}

	if( !hexptr )
		hexptr = str;

	for( ; *hexptr == ':' || *hexptr == ' '; hexptr++ )
		;

	readable.append(str, hexptr - str);
	cout << readable << endl;

	data.AppendHexString(hexptr);
}

int main(int argc, char *argv[])
{
	cout.sync_with_stdio(false);

	if( argc < 2 ) {
		Usage();
		return 0;
	}

	Barry::Data data;
	while( cin ) {
		std::string buff;
		getline(cin, buff);
		if( IsHexData(argv[1], buff.c_str()) ) {
			SplitHex(argv[1], buff.c_str(), data);
		}
		else {
			if( data.GetSize() ) {
				cout << data;
				data.Zap();
			}
			cout << buff << "\n";
		}
	}

	if( data.GetSize() ) {
		cout << data;
	}
}

