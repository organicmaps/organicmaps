#include "delimiters.hpp"

namespace search
{

bool Delimiters::operator()(strings::UniChar c) const
{
  // @TODO impement full unicode range delimiters table
  // latin table optimization
  if (c >= ' ' && c < '0')
    return true;
  switch (c)
  {
  case ':':
  case ';':
  case '<':
  case '=':
  case '>':
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
