#pragma once

#include "search/model.hpp"
#include "search/query_params.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace search
{
class QueryParams;

namespace impl
{
bool FullMatch(QueryParams::Token const & token, strings::UniString const & text);

bool PrefixMatch(QueryParams::Token const & token, strings::UniString const & text);
}  // namespace impl

// The order and numeric values are important here.  Please, check all
// use-cases before changing this enum.
enum NameScore
{
  NAME_SCORE_ZERO = 0,
  NAME_SCORE_SUBSTRING = 1,
  NAME_SCORE_PREFIX = 2,
  NAME_SCORE_FULL_MATCH = 3,

  NAME_SCORE_COUNT
};

// Returns true when |s| is a stop-word and may be removed from a query.
bool IsStopWord(strings::UniString const & s);

// Normalizes, simplifies and splits string, removes stop-words.
void PrepareStringForMatching(std::string const & name, std::vector<strings::UniString> & tokens);

template <typename Slice>
NameScore GetNameScore(std::string const & name, Slice const & slice)
{
  if (slice.Empty())
    return NAME_SCORE_ZERO;

  std::vector<strings::UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), Delimiters());
  return GetNameScore(tokens, slice);
}

template <typename Slice>
NameScore GetNameScore(std::vector<strings::UniString> const & tokens, Slice const & slice)
{
  if (slice.Empty())
    return NAME_SCORE_ZERO;

  size_t const n = tokens.size();
  size_t const m = slice.Size();

  bool const lastTokenIsPrefix = slice.IsPrefix(m - 1);

  NameScore score = NAME_SCORE_ZERO;
  for (size_t offset = 0; offset + m <= n; ++offset)
  {
    bool match = true;
    for (size_t i = 0; i < m - 1 && match; ++i)
      match = match && impl::FullMatch(slice.Get(i), tokens[offset + i]);
    if (!match)
      continue;

    bool const fullMatch = impl::FullMatch(slice.Get(m - 1), tokens[offset + m - 1]);
    bool const prefixMatch =
        lastTokenIsPrefix && impl::PrefixMatch(slice.Get(m - 1), tokens[offset + m - 1]);
    if (!fullMatch && !prefixMatch)
      continue;

    if (m == n && fullMatch)
      return NAME_SCORE_FULL_MATCH;

    if (offset == 0)
      score = max(score, NAME_SCORE_PREFIX);

    score = max(score, NAME_SCORE_SUBSTRING);
  }
  return score;
}

string DebugPrint(NameScore score);
}  // namespace search
