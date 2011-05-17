#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/stdint.hpp"

namespace utf8_string
{
  typedef bool (*IsDelimiterFuncT)(uint32_t);
  /// delimeters optimal for search
  bool IsSearchDelimiter(uint32_t symbol);
  bool Split(string const & str, vector<string> & out, IsDelimiterFuncT f = &IsSearchDelimiter);
}
