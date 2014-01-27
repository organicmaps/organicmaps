#include "message.hpp"

string DebugPrint(string const & t)
{
//  string res;
//  res.push_back('\'');
//  for (string::const_iterator it = t.begin(); it != t.end(); ++it)
//  {
//    static char const toHex[] = "0123456789abcdef";
//    unsigned char const c = static_cast<unsigned char>(*it);
//    if (c >= ' ' && c <= '~')
//      res.push_back(*it);
//    else
//    {
//      res.push_back('\\');
//      res.push_back(toHex[c >> 4]);
//      res.push_back(toHex[c & 0xf]);
//    }
//  }
//  res.push_back('\'');
  // Simply print UTF-8 string. Our computers (finally) supports it.
  return t;
}
