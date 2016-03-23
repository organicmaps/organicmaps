#include "search/v2/ranking_utils.hpp"

#include "std/algorithm.hpp"

using namespace strings;

namespace search
{
namespace v2
{
namespace impl
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
}  // namespace v2
}  // namespace search
