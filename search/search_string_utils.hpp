#pragma once
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"

namespace search
{

// This function should be used for all search strings normalization.
// It does some magic text transformation which greatly helps us to improve our search.
inline strings::UniString NormalizeAndSimplifyString(string const & s)
{
  using namespace strings;
  UniString uniString = MakeUniString(s);
  for (size_t i = 0; i < uniString.size(); ++i)
  {
    UniChar & c = uniString[i];
    switch (c)
    {
    // Replace "d with stroke" to simple d letter. Used in Vietnamese.
    // (unicode-compliant implementation leaves it unchanged)
    case 0x0110:
    case 0x0111: c = 'd'; break;
    // Replace small turkish dotless 'ı' with dotted 'i'.
    // Our own invented hack to avoid well-known Turkish I-letter bug.
    case 0x0131: c = 'i'; break;
    // Replace capital turkish dotted 'İ' with dotted lowercased 'i'.
    // Here we need to handle this case manually too, because default unicode-compliant implementation
    // of MakeLowerCase converts 'İ' to 'i' + 0x0307.
    case 0x0130: c = 'i'; break;
    // Some Danish-specific hacks.
    case 0x00d8:                    // Ø
    case 0x00f8: c = 'o'; break;    // ø
    case 0x0152:                    // Œ
    case 0x0153:                    // œ
      c = 'o';
      uniString.insert(uniString.begin() + (i++) + 1, 'e');
      break;
    case 0x00c6:                    // Æ
    case 0x00e6:                    // æ
      c = 'a';
      uniString.insert(uniString.begin() + (i++) + 1, 'e');
      break;
    }
  }

  MakeLowerCaseInplace(uniString);
  NormalizeInplace(uniString);

  // Remove accents that can appear after NFKD normalization.
  uniString.erase_if([](UniChar const & c)
  {
    // ̀  COMBINING GRAVE ACCENT
    // ́  COMBINING ACUTE ACCENT
    return (c == 0x0300 || c == 0x0301);
  });

  return uniString;

  /// @todo Restore this logic to distinguish и-й in future.
  /*
  // Just after lower casing is a correct place to avoid normalization for specific chars.
  static auto const isSpecificChar = [](UniChar c) -> bool
  {
    return c == 0x0439; // й
  };
  UniString result;
  result.reserve(uniString.size());
  for (auto i = uniString.begin(), end = uniString.end(); i != end;)
  {
    auto j = find_if(i, end, isSpecificChar);
    // We don't check if (j != i) because UniString and Normalize handle it correctly.
    UniString normString(i, j);
    NormalizeInplace(normString);
    result.insert(result.end(), normString.begin(), normString.end());
    if (j == end)
      break;
    result.push_back(*j);
    i = j + 1;
  }
  return result;
  */
}

template <class DelimsT, typename F>
void SplitUniString(strings::UniString const & uniS, F f, DelimsT const & delims)
{
  for (strings::TokenizeIterator<DelimsT> iter(uniS, delims); iter; ++iter)
    f(iter.GetUniString());
}

strings::UniString FeatureTypeToString(uint32_t type);

template <class ContainerT, class DelimsT>
bool TokenizeStringAndCheckIfLastTokenIsPrefix(strings::UniString const & s,
                                               ContainerT & tokens,
                                               DelimsT const & delimiter)
{
  SplitUniString(s, MakeBackInsertFunctor(tokens), delimiter);
  return !s.empty() && !delimiter(s.back());
}


template <class ContainerT, class DelimsT>
bool TokenizeStringAndCheckIfLastTokenIsPrefix(string const & s,
                                               ContainerT & tokens,
                                               DelimsT const & delimiter)
{
  return TokenizeStringAndCheckIfLastTokenIsPrefix(NormalizeAndSimplifyString(s),
                                                   tokens,
                                                   delimiter);
}

void GetStreetName(strings::SimpleTokenizer iter, string & streetName);
void GetStreetNameAsKey(string const & name, string & res);

}  // namespace search
