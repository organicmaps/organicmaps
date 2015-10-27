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

char hex_to_dec(char ch)
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'a')
    ch -= 32;
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

  const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
  const string::size_type SRC_LEN = sSrc.length();
  const unsigned char * const SRC_END = pSrc + SRC_LEN;
  // last decodable '%'
  const unsigned char * const SRC_LAST_DEC = SRC_END - 2;

  char * const pStart = new char[SRC_LEN];
  char * pEnd = pStart;

  while (pSrc < SRC_LAST_DEC)
  {
    if (*pSrc == '%')
    {
      char dec1 = hex_to_dec(*(pSrc + 1));
      char dec2 = hex_to_dec(*(pSrc + 2));
      if (-1 != dec1 && -1 != dec2)
      {
        *pEnd++ = (dec1 << 4) + dec2;
        pSrc += 2;
      }
    }
    else if (*pSrc == '_')
      *pEnd++ = ' ';
    else
      *pEnd++ = *pSrc;
    pSrc++;
  }

  // the last 2- chars
  while (pSrc < SRC_END)
    *pEnd++ = *pSrc++;

  string sResult(pStart, pEnd);
  delete [] pStart;
  return sResult;
}

string Metadata::GetWikiTitle() const
{
  string value = this->Get(FMD_WIKIPEDIA);
  return UriDecode(value);
}
}  // namespace feature
