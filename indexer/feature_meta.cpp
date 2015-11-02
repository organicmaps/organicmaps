#include "indexer/feature_meta.hpp"

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
  // This code is based on
  // http://www.codeguru.com/cpp/cpp/string/conversions/article.php/c12759
  //
  // Note from RFC1630: "Sequences which start with a percent
  // sign but are not followed by two hexadecimal characters
  // (0-9, A-F) are reserved for future extension"

  string result(sSrc.length(), 0);
  auto itResult = result.begin();

  for (auto it = sSrc.begin(); it != sSrc.end(); ++it)
  {
    if (*it == '%')
    {
      if (distance(it, sSrc.end()) > 2)
      {
        char dec1 = HexToDec(*(it + 1));
        char dec2 = HexToDec(*(it + 2));
        if (-1 != dec1 && -1 != dec2)
        {
          *itResult++ = (dec1 << 4) + dec2;
          it += 2;
          continue;
        }
      }
    }

    if (*it == '_')
      *itResult++ = ' ';
    else
      *itResult++ = *it;
  }

  result.resize(distance(result.begin(), itResult));
  return result;
}
}  // namespace

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

string Metadata::GetWikiTitle() const
{
  string value = this->Get(FMD_WIKIPEDIA);
  return UriDecode(value);
}
}  // namespace feature
