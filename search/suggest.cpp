#include "search/suggest.hpp"

#include "indexer/search_string_utils.hpp"

#include <algorithm>
#include <vector>

namespace search
{

std::string GetSuggestion(std::string const & name, QueryString const & query)
{
  auto const nTokens = NormalizeAndTokenizeString(name);

  bool prefixMatched = false;
  bool fullPrefixMatched = false;

  for (auto const & token : nTokens)
  {
    if (StartsWith(token, query.m_prefix))
    {
      prefixMatched = true;
      fullPrefixMatched = token.size() == query.m_prefix.size();
    }
  }

  // When |name| does not match prefix or when prefix equals to some
  // token of the |name| (for example, when user entered "Moscow"
  // without space at the end), we should not suggest anything.
  if (!prefixMatched || fullPrefixMatched)
    return {};

  std::string suggest;
  for (auto const & token : query.m_tokens)
  {
    /// @todo Process street shorts like: st, av, ne, w, ..
    if (std::find(nTokens.begin(), nTokens.end(), token) == nTokens.end())
    {
      suggest += strings::ToUtf8(token);
      suggest += ' ';
    }
  }

  return suggest + name + ' ';
}
}  // namespace search
