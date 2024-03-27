#include "urlencode.h"
#include <cassert>
#include <sstream>
#include <iomanip>

inline bool isUnreserved(char c)
{
    switch (c)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':

        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':

        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':

        case '-': case '.': case '_': case '~':
            return true;

        default:
            return false;
    }
}

inline bool isSubDelim(char c)
{
    switch (c)
    {
        case '!': case '$': case '&': case '\'': case '(':
        case ')': case '*': case '+': case ',':  case ';':
        case '=':
            return true;

        default:
            return false;
    }
}

std::string char2hex( char dec )
{
	char dig1 = (dec&0xF0)>>4;
	char dig2 = (dec&0x0F);
	if ( 0<= dig1 && dig1<= 9) dig1+=48;    //0,48 in ascii
	if (10<= dig1 && dig1<=15) dig1+=65-10; //A,65 in ascii
	if ( 0<= dig2 && dig2<= 9) dig2+=48;
	if (10<= dig2 && dig2<=15) dig2+=65-10;

    std::string r;
	r.append( &dig1, 1);
	r.append( &dig2, 1);
	return r;
}

std::string urlencode( const std::string &s, URLEncodeType enctype)
{
    std::stringstream escaped;

    std::string::const_iterator itStr = s.begin();
    for (; itStr != s.end(); ++itStr)
    {
        char c = *itStr;

        // Unreserved chars - never percent-encoded
        if (isUnreserved(c))
        {
            escaped << c;
            continue;
        }

        // Further on, the encoding depends on the context (where in the
        // URI we are, what type of URI, and which character).
        switch (enctype)
        {
            case URLEncode_Path:
                if (isSubDelim(c))
                {
                    escaped << c;
                    continue;
                }
                /* fall-through */

            case URLEncode_Everything:
                escaped << '%' << char2hex(c);
                break;

            default:
                assert(false && "Unknown urlencode type");
                break;
        }
    }

    return escaped.str();
}
