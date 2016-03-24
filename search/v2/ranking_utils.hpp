#pragma once

#include "search/search_query_params.hpp"
#include "search/v2/geocoder.hpp"
#include "search/v2/search_model.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
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

class TokensSlice
{
public:
  TokensSlice(SearchQueryParams const & params, size_t startToken, size_t endToken)
    : m_params(params), m_offset(startToken), m_size(endToken - startToken)
  {
    ASSERT_LESS_OR_EQUAL(startToken, endToken, ());
  }

  inline SearchQueryParams::TSynonymsVector const & Get(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_params.GetTokens(m_offset + i);
  }

  inline size_t Size() const { return m_size; }

  inline bool Empty() const { return Size() == 0; }

  inline bool IsPrefix(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_offset + i == m_params.m_tokens.size();
  }

private:
  SearchQueryParams const & m_params;
  size_t const m_offset;
  size_t const m_size;
};

class TokensSliceNoCategories
{
public:
  TokensSliceNoCategories(SearchQueryParams const & params, size_t startToken, size_t endToken)
    : m_params(params)
  {
    ASSERT_LESS_OR_EQUAL(startToken, endToken, ());

    m_indexes.reserve(endToken - startToken);
    for (size_t i = startToken; i < endToken; ++i)
    {
      if (!m_params.m_isCategorySynonym[i])
        m_indexes.push_back(i);
    }
  }

  inline SearchQueryParams::TSynonymsVector const & Get(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_params.GetTokens(m_indexes[i]);
  }

  inline size_t Size() const { return m_indexes.size(); }

  inline bool Empty() const { return Size() == 0; }

  inline bool IsPrefix(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_indexes[i] == m_params.m_tokens.size();
  }

private:
  SearchQueryParams const & m_params;
  vector<size_t> m_indexes;
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
