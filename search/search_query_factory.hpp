#pragma once

#include "search/search_query.hpp"
#include "search/suggest.hpp"

#if defined(USE_SEARCH_QUERY_V2)
#include "search/v2/search_query_v2.hpp"
#endif  // defined(USE_SEARCH_QUERY_V2)

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
#if defined(USE_SEARCH_QUERY_V2)
    return make_unique<v2::SearchQueryV2>(index, categories, suggests, infoGetter);
#else
    return make_unique<Query>(index, categories, suggests, infoGetter);
#endif  // defined(USE_SEARCH_QUERY_V2)
  }
};
}  // namespace search
