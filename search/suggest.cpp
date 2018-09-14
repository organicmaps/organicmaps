#include "search/suggest.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "search/common.hpp"

#include "base/stl_helpers.hpp"

#include <vector>

using namespace std;

namespace search
{
void GetSuggestion(RankerResult const & res, string const & query, QueryTokens const & paramTokens,
                   strings::UniString const & prefix, string & suggest)
{
  // Splits result's name.
  search::Delimiters delims;
  vector<strings::UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(res.GetName()), base::MakeBackInsertFunctor(tokens), delims);

  // Finds tokens that are already present in the input query.
  vector<bool> tokensMatched(tokens.size());
  bool prefixMatched = false;
  bool fullPrefixMatched = false;

  for (size_t i = 0; i < tokens.size(); ++i)
  {
    auto const & token = tokens[i];

    if (find(paramTokens.begin(), paramTokens.end(), token) != paramTokens.end())
    {
      tokensMatched[i] = true;
    }
    else if (StartsWith(token, prefix))
    {
      prefixMatched = true;
      fullPrefixMatched = token.size() == prefix.size();
    }
  }

  // When |name| does not match prefix or when prefix equals to some
  // token of the |name| (for example, when user entered "Moscow"
  // without space at the end), we should not suggest anything.
  if (!prefixMatched || fullPrefixMatched)
    return;

  suggest = DropLastToken(query);

  // Appends unmatched result's tokens to the suggestion.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokensMatched[i])
      continue;
    suggest.append(strings::ToUtf8(tokens[i]));
    suggest.push_back(' ');
  }
}
}  // namespace search
