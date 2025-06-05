#pragma once

#include <string>

namespace features_dumper
{
void DumpTypes(std::string const & fPath);
void DumpPrefixes(std::string const & fPath);

// Writes top maxTokensToShow tokens sorted by their
// frequency, i.e. by the number of features in
// an mwm that contain the token in their name.
void DumpSearchTokens(std::string const & fPath, size_t maxTokensToShow);

// Writes the names of all features in the locale provided by lang
// (e.g. "en", "ru", "sv"). If the locale is not recognized, writes all names
// preceded by their locales.
void DumpFeatureNames(std::string const & fPath, std::string const & lang);
}  // namespace features_dumper
