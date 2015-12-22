#include "search/search_tests_support/test_search_engine.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/scales.hpp"

#include "search/search_query.hpp"
#include "search/search_query_factory.hpp"

#include "platform/platform.hpp"

#include "std/unique_ptr.hpp"

namespace
{
class TestQuery : public search::Query
{
public:
  TestQuery(Index & index, CategoriesHolder const & categories,
            vector<search::Suggest> const & suggests, storage::CountryInfoGetter const & infoGetter)
    : search::Query(index, categories, suggests, infoGetter)
  {
  }

  // search::Query overrides:
  int GetQueryIndexScale(m2::RectD const & /* viewport */) const override
  {
    return scales::GetUpperScale();
  }
};

class TestSearchQueryFactory : public search::SearchQueryFactory
{
  // search::SearchQueryFactory overrides:
  unique_ptr<search::Query> BuildSearchQuery(Index & index, CategoriesHolder const & categories,
                                             vector<search::Suggest> const & suggests,
                                             storage::CountryInfoGetter const & infoGetter) override
  {
    return make_unique<TestQuery>(index, categories, suggests, infoGetter);
  }
};
}  // namespace

namespace search
{
namespace tests_support
{
TestSearchEngine::TestSearchEngine(string const & locale)
  : m_platform(GetPlatform())
  , m_infoGetter(m_platform.GetReader(PACKED_POLYGONS_FILE), m_platform.GetReader(COUNTRIES_FILE))
  , m_engine(*this, m_platform.GetReader(SEARCH_CATEGORIES_FILE_NAME), m_infoGetter, locale,
             make_unique<TestSearchQueryFactory>())
{
}

bool TestSearchEngine::Search(search::SearchParams const & params, m2::RectD const & viewport)
{
  return m_engine.Search(params, viewport);
}
}  // namespace tests_support
}  // namespace search
