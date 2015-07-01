#pragma once

#include "search_query.hpp"

#include "std/unique_ptr.hpp"

namespace search
{
class SearchQueryFactory
{
public:
  virtual ~SearchQueryFactory() = default;

  virtual unique_ptr<Query> BuildSearchQuery(
      Index const * index, CategoriesHolder const * categories,
      Query::StringsToSuggestVectorT const * stringsToSuggest,
      storage::CountryInfoGetter const * infoGetter)
  {
    return make_unique<Query>(index, categories, stringsToSuggest, infoGetter);
  }
};
}  // namespace search
