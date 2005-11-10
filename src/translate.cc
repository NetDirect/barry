#include <iostream>
#include <iomanip>
#include <sstream>

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
/*
	ios::fmtflags flags = cout.setf(ios::right);
	streamsize width = cout.width(14 + 16 * 3 + 1);
	cout.flags(flags);
	cout.width(width);
*/

	cout << setiosflags(ios::left) << setw(14 + 16 * 3 + 1) << str;
	cout << setw(0);
	str += 14;
	char *endpos = (char*) str;
	while( *endpos ) {
		int c = (int) strtol(str, &endpos, 16);
		if( isprint(c) )
			cout << (char)c;
		else
			cout << '.';
		str = endpos;
	}
	cout << '\n';

/*
	cout << str << ' ';
	istringstream iss(str+14);
	iss.setf(ios::hex);
	cout.setf(ios::hex);
	cout.width(2);
	cout.fill('0');
	while( iss ) {
		unsigned int val;
		iss >> val;
		cout << val << ' ';
	}
	cout << '\n';
*/
}

int main()
{
	cout.sync_with_stdio(false);

	while( cin ) {
		char buff[1024];
		cin.getline(buff, sizeof(buff));
		if( IsHexData(buff) )
			PrintHex(buff);
		else
			cout << buff << "\n";

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

