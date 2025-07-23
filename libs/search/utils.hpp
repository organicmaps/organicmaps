#pragma once

#include "search/common.hpp"
#include "search/feature_offset_match.hpp"
#include "search/token_slice.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie.hpp"

#include "geometry/rect2d.hpp"

#include "base/levenshtein_dfa.hpp"
#include "base/string_utils.hpp"

#include <functional>
#include <memory>
#include <vector>

class DataSource;
class MwmInfo;

namespace search
{
template <typename ToDo>
void ForEachCategoryType(StringSliceBase const & slice, Locales const & locales, CategoriesHolder const & categories,
                         ToDo && todo)
{
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    auto const & token = slice.Get(i);
    for (int8_t const locale : locales)
      categories.ForEachTypeByName(locale, token, [&todo, i](uint32_t type) { todo(i, type); });

    // Special case processing of 2 codepoints emoji (e.g. black guy on a bike).
    // Only emoji synonyms can have one codepoint.
    if (token.size() > 1)
    {
      categories.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::UniString(1, token[0]),
                                   [&todo, i](uint32_t type) { todo(i, type); });
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
  using Iterator = trie::MemTrieIterator<strings::UniString, base::VectorValues<uint32_t>>;

  auto const & trie = categories.GetNameToTypesTrie();
  Iterator const iterator(trie.GetRootIterator());

  for (size_t i = 0; i < slice.Size(); ++i)
  {
    /// @todo We build dfa twice for each token: here and in geocoder.cpp.
    /// A possible optimization is to build each dfa once and save it. Note that
    /// dfas for the prefix tokens differ, i.e. we ignore slice.IsPrefix(i) here.

    SearchTrieRequest<strings::LevenshteinDFA> request;
    /// @todo Shall we match prefix tokens for categories?
    request.m_names.push_back(BuildLevenshteinDFA_Category(slice.Get(i)));
    request.SetLangs(locales);

    MatchFeaturesInTrie(request, iterator, [](uint32_t) { return true; } /* filter */,
                        [&todo, i](uint32_t type, bool) { todo(i, type); } /* todo */);
  }
}

// Returns |true| and fills |types| if request specified by |slice| is categorial
// in any of the |locales| and |false| otherwise.
// We expect that categorial requests should mostly arise from clicking on a category
// button in the UI but also allow typing the category name without errors manually
// and not adding a space.
template <typename T>
bool FillCategories(QuerySliceOnRawStrings<T> const & slice, Locales const & locales,
                    CategoriesHolder const & catHolder, std::vector<uint32_t> & types)
{
  types.clear();
  catHolder.ForEachNameAndType([&](CategoriesHolder::Category::Name const & categorySynonym, uint32_t type)
  {
    if (!locales.Contains(static_cast<uint64_t>(categorySynonym.m_locale)))
      return;

    auto const categoryTokens = NormalizeAndTokenizeString(categorySynonym.m_name);

    if (slice.Size() != categoryTokens.size())
      return;

    for (size_t i = 0; i < slice.Size(); ++i)
      if (slice.Get(i) != categoryTokens[i])
        return;

    types.push_back(type);
  });

  return !types.empty();
}

// Returns classificator types for category with |name| and |locale|. For metacategories
// like "Hotel" returns all subcategories types.
std::vector<uint32_t> GetCategoryTypes(std::string const & name, std::string const & locale,
                                       CategoriesHolder const & categories);

using FeatureIndexCallback = std::function<void(FeatureID const &)>;
// Applies |fn| to each feature index of type from |types| in |rect|.
void ForEachOfTypesInRect(DataSource const & dataSource, std::vector<uint32_t> const & types, m2::RectD const & rect,
                          FeatureIndexCallback const & fn);

// Returns true iff |query| contains |categoryEn| synonym.
bool IsCategorialRequestFuzzy(std::string const & query, std::string const & categoryName);

template <typename DFA>
void FillRequestFromToken(QueryParams::Token const & token, SearchTrieRequest<DFA> & request)
{
  request.m_names.emplace_back(BuildLevenshteinDFA(token.GetOriginal()));
  // Allow misprints for original token only.
  token.ForEachSynonym([&request](strings::UniString const & s)
  { request.m_names.emplace_back(strings::LevenshteinDFA(s, 0 /* maxErrors */)); });
}
}  // namespace search
