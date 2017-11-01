#include "search/search_tests_support/test_search_engine.hpp"

#include "indexer/categories_holder.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

namespace search
{
namespace tests_support
{
TestSearchEngine::TestSearchEngine(unique_ptr<storage::CountryInfoGetter> infoGetter,
                                   Engine::Params const & params)
  : m_platform(GetPlatform())
  , m_infoGetter(move(infoGetter))
  , m_engine(*this, GetDefaultCategories(), *m_infoGetter, params)
{
}

TestSearchEngine::TestSearchEngine(Engine::Params const & params)
  : m_platform(GetPlatform())
  , m_infoGetter(storage::CountryInfoReader::CreateCountryInfoReader(m_platform))
  , m_engine(*this, GetDefaultCategories(), *m_infoGetter, params)
{
}

TestSearchEngine::~TestSearchEngine() {}

weak_ptr<::search::ProcessorHandle> TestSearchEngine::Search(::search::SearchParams const & params)
{
  return m_engine.Search(params);
}
}  // namespace tests_support
}  // namespace search
