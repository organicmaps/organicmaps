#include "search/suggest.hpp"

#include "indexer/search_string_utils.hpp"

#include <algorithm>
#include <vector>

namespace search
{

std::string GetSuggestion(std::string const & name, QueryString const & query)
{
  // Splits result's name.
  auto const tokens = NormalizeAndTokenizeString(name);

  // Finds tokens that are already present in the input query.
  std::vector<bool> tokensMatched(tokens.size());
  bool prefixMatched = false;
  bool fullPrefixMatched = false;

  for (size_t i = 0; i < tokens.size(); ++i)
  {
    auto const & token = tokens[i];

    if (std::find(query.m_tokens.begin(), query.m_tokens.end(), token) != query.m_tokens.end())
    {
      tokensMatched[i] = true;
    }
    else if (StartsWith(token, query.m_prefix))
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

  std::string suggest = DropLastToken(query.m_query);

  // Appends unmatched result's tokens to the suggestion.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokensMatched[i])
      continue;
    suggest.append(strings::ToUtf8(tokens[i]));
    suggest.push_back(' ');
  }

  return suggest;
}
}  // namespace search
