#pragma once

#include "search/common.hpp"
#include "search/feature_offset_match.hpp"
#include "search/token_slice.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie.hpp"

#include "base/levenshtein_dfa.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Index;
class MwmInfo;

namespace search
{
size_t GetMaxErrorsForToken(strings::UniString const & token);

strings::LevenshteinDFA BuildLevenshteinDFA(strings::UniString const & s);

template <typename ToDo>
void ForEachCategoryType(StringSliceBase const & slice, Locales const & locales,
                         CategoriesHolder const & categories, ToDo && todo)
{
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    auto const & token = slice.Get(i);
    for (int8_t const locale : locales)
      categories.ForEachTypeByName(locale, token, std::bind<void>(todo, i, std::placeholders::_1));

    // Special case processing of 2 codepoints emoji (e.g. black guy on a bike).
    // Only emoji synonyms can have one codepoint.
    if (token.size() > 1)
    {
      categories.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::UniString(1, token[0]),
                                   std::bind<void>(todo, i, std::placeholders::_1));
    }
  }
}

// Unlike ForEachCategoryType which extracts types by a token
// from |slice| by exactly matching it to a token in the name
// of a category, in the worst case this function has to loop through the tokens
// in all category synonyms in all |locales| in order to find a token
// whose edit distance is close enough to the required token from |slice|.
template <typename ToDo>
void ForEachCategoryTypeFuzzy(StringSliceBase const & slice, Locales const & locales,
                              CategoriesHolder const & categories, ToDo && todo)
{
  using Iterator = trie::MemTrieIterator<strings::UniString, ::base::VectorValues<uint32_t>>;

  auto const & trie = categories.GetNameToTypesTrie();
  Iterator const iterator(trie.GetRootIterator());

  for (size_t i = 0; i < slice.Size(); ++i)
  {
    // todo(@m, @y). We build dfa twice for each token: here and in geocoder.cpp.
    // A possible optimization is to build each dfa once and save it. Note that
    // dfas for the prefix tokens differ, i.e. we ignore slice.IsPrefix(i) here.
    SearchTrieRequest<strings::LevenshteinDFA> request;
    request.m_names.push_back(BuildLevenshteinDFA(slice.Get(i)));
    request.SetLangs(locales);

    MatchFeaturesInTrie(request, iterator, [&](uint32_t /* type */) { return true; } /* filter */,
                        std::bind<void>(todo, i, std::placeholders::_1));
  }
}

// Returns whether the request specified by |slice| is categorial
// in any of the |locales|. We expect that categorial requests should
// mostly arise from clicking on a category button in the UI.
// It is assumed that typing a word that matches a category's name
// and a space after it means that no errors were made.
template <typename T>
bool IsCategorialRequest(QuerySliceOnRawStrings<T> const & slice, Locales const & locales,
                         CategoriesHolder const & catHolder)
{
  if (slice.Size() != 1 || slice.HasPrefixToken())
    return false;

  bool found = false;
  auto token = slice.Get(0);
  catHolder.ForEachName([&](CategoriesHolder::Category::Name const & categorySynonym) {
    if (!locales.Contains(static_cast<uint64_t>(categorySynonym.m_locale)))
      return;

    if (token != search::NormalizeAndSimplifyString(categorySynonym.m_name))
      return;

    found = true;
  });

  return found;
}

MwmSet::MwmHandle FindWorld(Index const &index,
                            std::vector<std::shared_ptr<MwmInfo>> const &infos);
MwmSet::MwmHandle FindWorld(Index const & index);
}  // namespace search
