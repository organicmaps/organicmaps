#pragma once

#include "search/model.hpp"
#include "search/query_params.hpp"
#include "search/utils.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/levenshtein_dfa.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace search
{
class QueryParams;

struct ErrorsMade
{
  static size_t constexpr kInfiniteErrors = std::numeric_limits<size_t>::max();

  ErrorsMade() = default;
  explicit ErrorsMade(size_t errorsMade) : m_errorsMade(errorsMade) {}

  bool IsValid() const { return m_errorsMade != kInfiniteErrors; }

  template <typename Fn>
  static ErrorsMade Combine(ErrorsMade const & lhs, ErrorsMade const & rhs, Fn && fn)
  {
    if (!lhs.IsValid())
      return rhs;
    if (!rhs.IsValid())
      return lhs;
    return ErrorsMade(fn(lhs.m_errorsMade, rhs.m_errorsMade));
  }

  static ErrorsMade Min(ErrorsMade const & lhs, ErrorsMade const & rhs)
  {
    return Combine(lhs, rhs, [](size_t u, size_t v) { return std::min(u, v); });
  }

  friend ErrorsMade operator+(ErrorsMade const & lhs, ErrorsMade const & rhs)
  {
    return Combine(lhs, rhs, [](size_t u, size_t v) { return u + v; });
  }

  ErrorsMade & operator+=(ErrorsMade const & rhs)
  {
    *this = *this + rhs;
    return *this;
  }

  bool operator==(ErrorsMade const & rhs) const { return m_errorsMade == rhs.m_errorsMade; }

  size_t m_errorsMade = kInfiniteErrors;
};

string DebugPrint(ErrorsMade const & errorsMade);

namespace impl
{
bool FullMatch(QueryParams::Token const & token, strings::UniString const & text);

bool PrefixMatch(QueryParams::Token const & token, strings::UniString const & text);

// Returns the minimum number of errors needed to match |text| with
// any of the |tokens|.  If it's not possible in accordance with
// GetMaxErrorsForToken(|text|), returns kInfiniteErrors.
ErrorsMade GetMinErrorsMade(std::vector<strings::UniString> const & tokens,
                            strings::UniString const & text);
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

// Returns total number of errors that were made during matching
// feature |tokens| by a query - query tokens are in |slice|.
template <typename Slice>
ErrorsMade GetErrorsMade(std::vector<strings::UniString> const & tokens, Slice const & slice)
{
  ErrorsMade totalErrorsMade;

  for (size_t i = 0; i < slice.Size(); ++i)
  {
    ErrorsMade errorsMade;
    slice.Get(i).ForEach([&](strings::UniString const & s) {
      errorsMade = ErrorsMade::Min(errorsMade, impl::GetMinErrorsMade(tokens, s));
    });

    totalErrorsMade += errorsMade;
  }

  return totalErrorsMade;
}

template <typename Slice>
ErrorsMade GetErrorsMade(std::string const & s, Slice const & slice)
{
  return GetErrorsMade({strings::MakeUniString(s)}, slice);
}
}  // namespace search
