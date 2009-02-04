/*
 * Encode or decode file as MIME base64 (RFC 1341)
 * Public domain by John Walker, August 11 1997
 *           http://www.fourmilab.ch/
 * Modified slightly for the Citadel/UX system, June 1999
 *
 * Taken from the Citadel/UX GPL source tree, at version 6.01
 * Modified into a C++ API by Chris Frey for Net Direct Inc., November 2005
 *           http://www.netdirect.ca/
 *
 */

#include "base64.h"
#include <string>
#include <iterator>

#define TRUE  1
#define FALSE 0

#define LINELEN 72		      /* Encoded line length (max 76) */

typedef unsigned char byte;	      /* Byte type */

static byte dtable[256];	      /* Encode / decode table */
//static char eol[] = "\r\n";           /* End of line sequence */
static int errcheck = TRUE;	      /* Check decode input for errors ? */


/*  INCHAR  --	Return next character from input  */

class base64_input
{
	std::string::const_iterator begin, end;
public:
	base64_input(const std::string &input)
		: begin(input.begin()), end(input.end()) {}

	int operator()()
	{
		if (begin == end) {
			return EOF;
		}
		return *begin++;
	}
};


/*  OCHAR  --  Output an encoded character, inserting line breaks
	       where required.	*/

class base64_output
{
	std::back_insert_iterator<std::string> insert;
	int linelength;			/* Length of encoded output line */

public:
	base64_output(std::string &output)
		: insert(back_inserter(output)),
		linelength(0)
	{}

	void operator()(int c)
	{
		if (linelength >= LINELEN) {
			*insert++ = '\n';
			*insert++ = ' ';
			linelength = 0;
		}
		*insert++ = (unsigned char) c;
		linelength++;
	}
};

/*  ENCODE  --	Encode binary file into base64.  */

static bool encode(base64_input &inchar, base64_output &ochar)
{
    int i, hiteof = FALSE;

    /*	Fill dtable with character encodings.  */

    for (i = 0; i < 26; i++) {
        dtable[i] = 'A' + i;
        dtable[26 + i] = 'a' + i;
    }
    for (i = 0; i < 10; i++) {
        dtable[52 + i] = '0' + i;
    }
    dtable[62] = '+';
    dtable[63] = '/';

    while (!hiteof) {
	byte igroup[3], ogroup[4];
	int c, n;

	igroup[0] = igroup[1] = igroup[2] = 0;
	for (n = 0; n < 3; n++) {
	    c = inchar();
	    if (c == EOF) {
		hiteof = TRUE;
		break;
	    }
	    igroup[n] = (byte) c;
	}
	if (n > 0) {
	    ogroup[0] = dtable[igroup[0] >> 2];
	    ogroup[1] = dtable[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
	    ogroup[2] = dtable[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
	    ogroup[3] = dtable[igroup[2] & 0x3F];

            /* Replace characters in output stream with "=" pad
	       characters if fewer than three characters were
	       read from the end of the input stream. */

	    if (n < 3) {
                ogroup[3] = '=';
		if (n < 2) {
                    ogroup[2] = '=';
		}
	    }
	    for (i = 0; i < 4; i++) {
		ochar(ogroup[i]);
	    }
	}
    }
    return true;
}

/*  INSIG  --  Return next significant input  */

static int insig(base64_input &inchar)
{
    int c;

    /*CONSTANTCONDITION*/
    while (TRUE) {
	c = inchar();
        if (c == EOF || (c > ' ')) {
	    return c;
	}
    }
    /*NOTREACHED*/
}

/*  DECODE  --	Decode base64.	*/

static bool decode(base64_input &inchar, base64_output &ochar)
{
    int i;

    for (i = 0; i < 255; i++) {
	dtable[i] = 0x80;
    }
    for (i = 'A'; i <= 'Z'; i++) {
        dtable[i] = 0 + (i - 'A');
    }
    for (i = 'a'; i <= 'z'; i++) {
        dtable[i] = 26 + (i - 'a');
    }
    for (i = '0'; i <= '9'; i++) {
        dtable[i] = 52 + (i - '0');
    }
    dtable[(int)'+'] = 62;
    dtable[(int)'/'] = 63;
    dtable[(int)'='] = 0;

    /*CONSTANTCONDITION*/
    while (TRUE) {
	byte a[4], b[4], o[3];

	for (i = 0; i < 4; i++) {
	    int c = insig(inchar);

	    if (c == EOF) {
                // fprintf(stderr, "Input file incomplete.\n");
		return false;
	    }
	    if (dtable[c] & 0x80) {
		if (errcheck) {
                    //fprintf(stderr, "Illegal character '%c' in input file.\n", c);
		    return false;
		}
		/* Ignoring errors: discard invalid character. */
		i--;
		continue;
	    }
	    a[i] = (byte) c;
	    b[i] = (byte) dtable[c];
	}
	o[0] = (b[0] << 2) | (b[1] >> 4);
	o[1] = (b[1] << 4) | (b[2] >> 2);
	o[2] = (b[2] << 6) | b[3];
        i = a[2] == '=' ? 1 : (a[3] == '=' ? 2 : 3);
	for (int w = 0; w < i; w++ )
	    ochar(o[w]);
	if (i < 3) {
	    return true;
	}
    }
}

// in-memory encode / decode API
bool base64_encode(const std::string &in, std::string &out)
{
	out.clear();
	base64_input input(in);
	base64_output output(out);
	return encode(input, output);
}

bool base64_decode(const std::string &in, std::string &out)
{
	out.clear();
	base64_input input(in);
	base64_output output(out);
	return decode(input, output);
}


#ifdef __TEST_MODE__

#include <iostream>
using namespace std;

/*  Main program  */

int main()
{
	string test = "This is a test.", encoded, decoded;
	base64_encode(test, encoded);
	base64_decode(encoded, decoded);
	if( test != decoded )
		cerr << "Test failed" << endl;
	else
		cerr << "Success" << endl;
}

#endif

