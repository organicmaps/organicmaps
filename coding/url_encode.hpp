#pragma once

#include "hex.hpp"

#include "../std/string.hpp"

inline string UrlEncode(string const & rawUrl)
{
  string result(rawUrl);
  for (size_t i = 0; i < result.size(); ++i)
  {
    char const c = result[i];
    if (c < '-' || c == '/' || (c > '9' && c < 'A') || (c > 'Z' && c < '_')
        || c == '`' || (c > 'z' && c < '~') || c > '~')
    {
      string hexStr("%");
      hexStr += NumToHex(c);
      result.replace(i, 1, hexStr);
      i += 2;
    }
  }
  return result;
}

inline string UrlDecode(string const & encodedUrl)
{
  string result;
  for (size_t i = 0; i < encodedUrl.size(); ++i)
  {
    if (encodedUrl[i] == '%')
    {
      result += FromHex(encodedUrl.substr(i + 1, 2));
      i += 2;
    }
    else
      result += encodedUrl[i];
  }
  return result;
}
