#pragma once

#include "../std/string.hpp"

namespace base64
{
  string encode(string const & rawBytes);
  string decode(string const & base64Chars);
}
