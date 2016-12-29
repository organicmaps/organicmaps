#include "message.hpp"

#include "std/target_os.hpp"


std::string DebugPrint(std::string const & t)
{
#ifdef OMIM_OS_WINDOWS
  string res;
  res.push_back('\'');
  for (string::const_iterator it = t.begin(); it != t.end(); ++it)
  {
    static char const toHex[] = "0123456789abcdef";
    unsigned char const c = static_cast<unsigned char>(*it);
    if (c >= ' ' && c <= '~')
      res.push_back(*it);
    else
    {
      res.push_back('\\');
      res.push_back(toHex[c >> 4]);
      res.push_back(toHex[c & 0xf]);
    }
  }
  res.push_back('\'');
  return res;

#else
  // Assume like UTF8 string.
  return t;
#endif
}
