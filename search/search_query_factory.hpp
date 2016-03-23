#pragma once

#include "search/suggest.hpp"
#include "search/v2/search_query_v2.hpp"

#include "std/unique_ptr.hpp"

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
class SearchQueryFactory
{
public:
  virtual ~SearchQueryFactory() = default;

  virtual unique_ptr<Query> BuildSearchQuery(Index & index, CategoriesHolder const & categories,
                                             vector<Suggest> const & suggests,
                                             storage::CountryInfoGetter const & infoGetter)
  {
    return make_unique<v2::SearchQueryV2>(index, categories, suggests, infoGetter);
  }
};
}  // namespace search
