#include "utf8_string.hpp"

#include "../std/iterator.hpp"

#include "../3party/utfcpp/source/utf8/unchecked.h"

namespace utf8_string
{
  bool Split(string const & str, vector<string> & out, IsDelimiterFuncT f)
  {
    out.clear();
    string::const_iterator curr = str.begin();
    string::const_iterator end = str.end();
    string word;
    back_insert_iterator<string> inserter = back_inserter(word);
    while (curr != end)
    {
      uint32_t symbol = ::utf8::unchecked::next(curr);
      if (f(symbol))
      {
        if (!word.empty())
        {
          out.push_back(word);
          word.clear();
          inserter = back_inserter(word);
        }
      }
      else
      {
        inserter = utf8::unchecked::append(symbol, inserter);
      }
    }
    if (!word.empty())
      out.push_back(word);
    return !out.empty();
  }

  bool IsSearchDelimiter(uint32_t symbol)
  {
    // latin table optimization
    if (symbol >= ' ' && symbol < '0')
      return true;

    switch (symbol)
    {
    case ':':
    case ';':
    case '[':
    case ']':
    case '\\':
    case '^':
    case '_':
    case '`':
    case '{':
    case '}':
    case '|':
    case '~':
    case 0x0336:
      return true;
    }
    return false;
  }
}
