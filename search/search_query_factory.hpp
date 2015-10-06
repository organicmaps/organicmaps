#pragma once

#include "search_query.hpp"
#include "suggest.hpp"

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
    return make_unique<Query>(index, categories, suggests, infoGetter);
  }
};
}  // namespace search
