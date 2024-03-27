#ifndef __URLENCODE_H__
#define __URLENCODE_H__

#include <iostream>
#include <string>

std::string char2hex( char dec );
enum URLEncodeType {
    URLEncode_Everything,
    URLEncode_Path,
    URLEncode_QueryKey = URLEncode_Everything, /* bkwds compatibility */
    URLEncode_QueryValue = URLEncode_Everything,
};
std::string urlencode( const std::string &s, URLEncodeType enctype );

#endif // __URLENCODE_H__
