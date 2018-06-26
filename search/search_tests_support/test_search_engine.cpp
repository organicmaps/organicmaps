#include "search/search_tests_support/test_search_engine.hpp"

#include "indexer/categories_holder.hpp"

#include "platform/platform.hpp"

#include <utility>

using namespace std;

namespace search
{
namespace tests_support
{
TestSearchEngine::TestSearchEngine(DataSource & dataSource,
                                   unique_ptr<storage::CountryInfoGetter> infoGetter,
                                   Engine::Params const & params)
  : m_infoGetter(move(infoGetter))
  , m_engine(dataSource, GetDefaultCategories(), *m_infoGetter, params)
{
}

TestSearchEngine::TestSearchEngine(DataSource & dataSource, Engine::Params const & params)
  : m_infoGetter(storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform()))
  , m_engine(dataSource, GetDefaultCategories(), *m_infoGetter, params)
{
}

weak_ptr<::search::ProcessorHandle> TestSearchEngine::Search(::search::SearchParams const & params)
{
  return m_engine.Search(params);
}
}  // namespace tests_support
}  // namespace search
