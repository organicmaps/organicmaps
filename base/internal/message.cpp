#include "message.hpp"

#include <utf8/unchecked.h>

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
  utf8::unchecked::utf32to8(utf32.begin(), utf32.end(), std::back_inserter(utf8));
  return utf8;
}
}  // namespace internal
