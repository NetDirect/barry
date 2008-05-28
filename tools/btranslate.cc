/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <string.h>
#include <stdlib.h>
#include <limits.h>

using namespace std;

bool IsHexData(const char *str)
{
	for( int i = 0; i < 4 && *str; str++, i++ )
		if( *str != ' ' )
			return false;

	for( int i = 0; i < 8 && *str; str++, i++ )
		if( !isdigit(*str) && !(*str >= 'a' && *str <= 'f') )
			return false;

	if( *str != ':' )
		return false;

	return true;
}

void PrintHex(const char *str)
{
	cout << setiosflags(ios::left) << setw(14 + 16 * 3 + 1) << str;
	cout << setw(0);
	str += 14;
	char *endpos = (char*) str;
	while( *endpos ) {
		int c = (int) strtol(str, &endpos, 16);
		if( c == LONG_MIN || c == LONG_MAX )
			break;
		if( isprint(c) )
			cout << (char)c;
		else
			cout << '.';
		str = endpos;
	}
	cout << '\n';
}

int main()
{
	cout.sync_with_stdio(false);

	while( cin ) {
		char buff[1024];
		cin.getline(buff, sizeof(buff));
		if( IsHexData(buff) ) {
			// strip whitespace
			int sln = strlen(buff);
			while( sln && (buff[sln] == 0 || isspace(buff[sln])) ){
				buff[sln--] = 0;
			}
			PrintHex(buff);
		}
		else {
			cout << buff << "\n";
		}

		if( cin.fail() && !cin.eof() ) {
			// getline busted its buffer... discard the
			// rest of the line.
			while( cin.fail() && !cin.eof() ) {
				cin.clear();
				cin.getline(buff, sizeof(buff));
			}
		}
	}
}

