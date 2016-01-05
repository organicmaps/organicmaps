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

  // Writes the names of all features in the locale provided by lang
  // (e.g. "en", "ru", "sv"). If the locale is not recognized, writes all names
  // preceded by locale.
  void DumpFeatureNames(string const & fPath, string const & lang);
}
