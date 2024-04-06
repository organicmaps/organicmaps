#include "message.hpp"

#include "std/target_os.hpp"

#include <utf8/unchecked.h>

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

namespace internal
{
std::string ToUtf8(std::u16string_view utf16)
{
  std::string utf8;
  utf8::unchecked::utf16to8(utf16.begin(), utf16.end(), std::back_inserter(utf8));
  return utf8;
}

std::string ToUtf8(std::u32string_view utf32)
{
  std::string utf8;
  utf8::unchecked::utf32to8(utf32.begin(), utf32.end(), utf8.begin());
  return utf8;
}
}  // namespace internal
