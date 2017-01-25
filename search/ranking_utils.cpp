#include "search/ranking_utils.hpp"

#include "std/algorithm.hpp"

using namespace strings;

namespace search
{
namespace impl
{
bool FullMatch(QueryParams::Token const & token, UniString const & text)
{
  if (token.m_original == text)
    return true;
  auto const & synonyms = token.m_synonyms;
  return find(synonyms.begin(), synonyms.end(), text) != synonyms.end();
}

bool PrefixMatch(QueryParams::Token const & token, UniString const & text)
{
  if (StartsWith(text, token.m_original))
    return true;

  for (auto const & synonym : token.m_synonyms)
  {
    if (StartsWith(text, synonym))
      return true;
  }
  return false;
}
}  // namespace impl

string DebugPrint(NameScore score)
{
  switch (score)
  {
  case NAME_SCORE_ZERO: return "Zero";
  case NAME_SCORE_SUBSTRING_PREFIX: return "Substring Prefix";
  case NAME_SCORE_SUBSTRING: return "Substring";
  case NAME_SCORE_FULL_MATCH_PREFIX: return "Full Match Prefix";
  case NAME_SCORE_FULL_MATCH: return "Full Match";
  case NAME_SCORE_COUNT: return "Count";
  }
  return "Unknown";
}

}  // namespace search
