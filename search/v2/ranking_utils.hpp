#pragma once

#include "search/search_query_params.hpp"
#include "search/v2/geocoder.hpp"
#include "search/v2/search_model.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
struct SearchQueryParams;

namespace v2
{
namespace impl
{
bool Match(vector<strings::UniString> const & tokens, strings::UniString const & token);

bool PrefixMatch(vector<strings::UniString> const & prefixes, strings::UniString const & token);
}  // namespace impl

// The order and numeric values are important here.  Please, check all
// use-cases before changing this enum.
enum NameScore
{
  NAME_SCORE_ZERO = 0,
  NAME_SCORE_SUBSTRING_PREFIX = 1,
  NAME_SCORE_SUBSTRING = 2,
  NAME_SCORE_FULL_MATCH_PREFIX = 3,
  NAME_SCORE_FULL_MATCH = 4,

  NAME_SCORE_COUNT
};

template <typename TSlice>
NameScore GetNameScore(string const & name, TSlice const & slice)
{
  if (slice.Empty())
    return NAME_SCORE_ZERO;

  vector<strings::UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), Delimiters());
  return GetNameScore(tokens, slice);
}

template <typename TSlice>
NameScore GetNameScore(vector<strings::UniString> const & tokens, TSlice const & slice)
{
  if (slice.Empty())
    return NAME_SCORE_ZERO;

  size_t const n = tokens.size();
  size_t const m = slice.Size();

  bool const lastTokenIsPrefix = slice.IsPrefix(m - 1);

  NameScore score = NAME_SCORE_ZERO;
  for (int offset = 0; offset + m <= n; ++offset)
  {
    bool match = true;
    for (int i = 0; i < m - 1 && match; ++i)
      match = match && impl::Match(slice.Get(i), tokens[offset + i]);
    if (!match)
      continue;

    if (impl::Match(slice.Get(m - 1), tokens[offset + m - 1]))
    {
      if (m == n)
        return NAME_SCORE_FULL_MATCH;
      score = max(score, NAME_SCORE_SUBSTRING);
    }
    if (lastTokenIsPrefix && impl::PrefixMatch(slice.Get(m - 1), tokens[offset + m - 1]))
    {
      if (m == n)
        return NAME_SCORE_FULL_MATCH_PREFIX;
      score = max(score, NAME_SCORE_SUBSTRING_PREFIX);
    }
  }
  return score;
}

string DebugPrint(NameScore score);
}  // namespace v2
}  // namespace search
