#include "indexer/feature_meta.hpp"

namespace feature
{
string Metadata::GetWikiURL() const
{
  string value = this->Get(FMD_WIKIPEDIA);
  string::size_type i = value.find(':');
  if (i == string::npos)
    return string();
  return "https://" + value.substr(0, i) + ".wikipedia.org/wiki/" + value.substr(i + 1);
}

namespace
{
char HexToDec(char ch)
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'a')
    ch -= 'a' - 'A';
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}

string UriDecode(string const & sSrc)
{
  // This code was slightly modified from
  // http://www.codeguru.com/cpp/cpp/string/conversions/article.php/c12759
  //
  // Note from RFC1630: "Sequences which start with a percent
  // sign but are not followed by two hexadecimal characters
  // (0-9, A-F) are reserved for future extension"

  unsigned char const * pSrc = (unsigned char const *)sSrc.c_str();
  string::size_type const srcLen = sSrc.length();
  unsigned char const * const srcEnd = pSrc + srcLen;
  // last decodable '%'
  unsigned char const * const srcLastDec = srcEnd - 2;

  char * const pStart = new char[srcLen];
  char * pEnd = pStart;

  while (pSrc < srcEnd)
  {
    if (*pSrc == '%')
    {
      if (pSrc < srcLastDec)
      {
        char dec1 = HexToDec(*(pSrc + 1));
        char dec2 = HexToDec(*(pSrc + 2));
        if (-1 != dec1 && -1 != dec2)
        {
          *pEnd++ = (dec1 << 4) + dec2;
          pSrc += 3;
          continue;
        }
      }
    }

    if (*pSrc == '_')
      *pEnd++ = ' ';
    else
      *pEnd++ = *pSrc;
    pSrc++;
  }

  string sResult(pStart, pEnd);
  delete [] pStart;
  return sResult;
}
}

string Metadata::GetWikiTitle() const
{
  string value = this->Get(FMD_WIKIPEDIA);
  return UriDecode(value);
}
}  // namespace feature
