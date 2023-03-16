#include "urlencode.h"
#include <cassert>

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

std::string urlencode( const std::string &c, URLEncodeType enctype)
{

    std::string escaped;
	int max = c.length();
	for(int i=0; i<max; i++)
	{
            // Unreserved chars
		if ( (48 <= c[i] && c[i] <= 57) ||//0-9
			(65 <= c[i] && c[i] <= 90) ||//ABC...XYZ
			(97 <= c[i] && c[i] <= 122) || //abc...xyz
			(c[i]=='~' || c[i]=='-' || c[i]=='_' || c[i]=='.')
			)
		{
			escaped.append( &c[i], 1);
		}
                else if (c[i] != ':' && c[i] != '/' && c[i] != '?' && c[i] != '#' &&
                    c[i] != '[' && c[i] != ']' && c[i] != '@' && c[i] != '%' &&

                    c[i] != '!' && c[i] != '$' && c[i] != '&' && c[i] != '\'' &&
                    c[i] != '(' && c[i] != ')' && c[i] != '*' && c[i] != '+' &&
                    c[i] != ',' && c[i] != ';' && c[i] != '=')
                {
                    // Characters not in unreserved (first if block) and not in
                    // the reserved set are always encoded.
                    escaped.append("%");
                    escaped.append( char2hex(c[i]) );//converts char 255 to string "FF"
                }
		else
		{
                    // Finally, the reserved set. Encoding here depends on the
                    // context (where in the URI we are, what type of URI, and
                    // which character).

                    bool enc = false;

                    // Always encode reserved gen-delims + '%' (which always
                    // needs encoding
                    if (c[i] == ':' || c[i] == '/' || c[i] == '?' || c[i] == '#' ||
                        c[i] == '[' || c[i] == ']' || c[i] == '@' || c[i] == '%')
                    {
                        enc = true;
                    }
                    else {
                        switch (enctype) {
                          case URLEncode_Everything:
                            enc = true;
                            break;
                          case URLEncode_Path:
                            // Only reserved sub-delim that needs encoding is %,
                            // taken care of above. Otherwise, leave unencoded
                            enc = false;
                            break;
                          case URLEncode_QueryKey:
                            if (c[i] == '&' ||
                                c[i] == '+' ||
                                c[i] == '=')
                                enc = true;
                            else
                                enc = false;
                            break;
                          case URLEncode_QueryValue:
                            if (c[i] == '&' ||
                                c[i] == '+')
                                enc = true;
                            else
                                enc = false;
                            break;
                          default:
                            assert(false && "Unknown urlencode type");
                            break;
                        }
                    }

                    if (enc) {
                        escaped.append("%");
                        escaped.append( char2hex(c[i]) );//converts char 255 to string "FF"
                    } else {
			escaped.append( &c[i], 1);
                    }
		}
	}
	return escaped;
}
