#include "search/v2/ranking_utils.hpp"

#include "search/search_query_params.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

using namespace strings;

namespace search
{
namespace v2
{
namespace
{
bool Match(vector<UniString> const & tokens, UniString const & token)
{
  return find(tokens.begin(), tokens.end(), token) != tokens.end();
}

bool PrefixMatch(vector<UniString> const & prefixes, UniString const & token)
{
  for (auto const & prefix : prefixes)
  {
    if (StartsWith(token, prefix))
      return true;
  }
  return false;
}
}  // namespace

NameScore GetNameScore(string const & name, SearchQueryParams const & params, size_t startToken,
                       size_t endToken)
{
  if (startToken >= endToken)
    return NAME_SCORE_ZERO;

  vector<UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), Delimiters());
  return GetNameScore(tokens, params, startToken, endToken);
}

NameScore GetNameScore(vector<UniString> const & tokens, SearchQueryParams const & params,
                       size_t startToken, size_t endToken)
{
  if (startToken >= endToken)
    return NAME_SCORE_ZERO;

  size_t const n = tokens.size();
  size_t const m = endToken - startToken;

  bool const lastTokenIsPrefix = (endToken == params.m_tokens.size() + 1);

  NameScore score = NAME_SCORE_ZERO;
  for (int offset = 0; offset + m <= n; ++offset)
  {
    bool match = true;
    for (int i = 0; i + 1 < m && match; ++i)
      match = match && Match(params.GetTokens(startToken + i), tokens[offset + i]);
    if (!match)
      continue;

    if (Match(params.GetTokens(endToken - 1), tokens[offset + m - 1]))
    {
      if (m == n)
        return NAME_SCORE_FULL_MATCH;
      score = max(score, NAME_SCORE_SUBSTRING);
    }
    if (lastTokenIsPrefix && PrefixMatch(params.GetTokens(endToken - 1), tokens[offset + m - 1]))
    {
      if (m == n)
        return NAME_SCORE_FULL_MATCH_PREFIX;
      score = max(score, NAME_SCORE_SUBSTRING_PREFIX);
    }
  }
  return score;
}

string DebugPrint(NameScore score)
{
  switch (score)
  {
  case NAME_SCORE_ZERO: return "Zero";
  case NAME_SCORE_SUBSTRING_PREFIX: return "Substring Prefix";
  case NAME_SCORE_SUBSTRING: return "Substring";
  case NAME_SCORE_FULL_MATCH_PREFIX: return "Full Match Prefix";
  case NAME_SCORE_FULL_MATCH: return "Full Match";
  }
  return "Unknown";
}
}  // namespace v2
}  // namespace search
