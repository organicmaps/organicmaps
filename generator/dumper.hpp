#pragma once

#include "std/string.hpp"

namespace feature
{
  void DumpTypes(string const & fPath);
  void DumpPrefixes(string const & fPath);

  // Writes top maxTokensToShow tokens sorted by their
  // frequency, i.e. by the number of features in
  // an mwm that contain the token in their name.
  void DumpSearchTokens(string const & fPath, size_t maxTokensToShow);
}
