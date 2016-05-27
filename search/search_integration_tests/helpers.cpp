#include "search/search_integration_tests/helpers.hpp"

#include "search/processor_factory.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

namespace search
{
// TestWithClassificator ---------------------------------------------------------------------------
TestWithClassificator::TestWithClassificator()
{
  GetStyleReader().SetCurrentStyle(MapStyleMerged);
  classificator::Load();
}

// SearchTest --------------------------------------------------------------------------------------
SearchTest::SearchTest()
  : m_platform(GetPlatform())
  , m_scopedLog(LDEBUG)
  , m_engine(make_unique<storage::CountryInfoGetterForTesting>(), make_unique<ProcessorFactory>(),
             Engine::Params())
{
}

SearchTest::~SearchTest()
{
  for (auto const & file : m_files)
    Cleanup(file);
}

void SearchTest::RegisterCountry(string const & name, m2::RectD const & rect)
{
  auto & infoGetter =
      static_cast<storage::CountryInfoGetterForTesting &>(m_engine.GetCountryInfoGetter());
  infoGetter.AddCountry(storage::CountryDef(name, rect));
}

bool SearchTest::ResultsMatch(string const & query,
                              vector<shared_ptr<tests_support::MatchingRule>> const & rules)
{
  return ResultsMatch(query, "en" /* locale */, rules);
}

bool SearchTest::ResultsMatch(string const & query,
                              string const & locale,
                              vector<shared_ptr<tests_support::MatchingRule>> const & rules)
{
  tests_support::TestSearchRequest request(m_engine, query, locale, Mode::Everywhere, m_viewport);
  request.Wait();
  return MatchResults(m_engine, rules, request.Results());
}

bool SearchTest::ResultsMatch(string const & query, Mode mode,
                              vector<shared_ptr<tests_support::MatchingRule>> const & rules)
{
  tests_support::TestSearchRequest request(m_engine, query, "en", mode, m_viewport);
  request.Wait();
  return MatchResults(m_engine, rules, request.Results());
}

size_t SearchTest::CountFeatures(m2::RectD const & rect)
{
  size_t count = 0;
  auto counter = [&count](const FeatureType & /* ft */)
  {
    ++count;
  };
  m_engine.ForEachInRect(counter, rect, scales::GetUpperScale());
  return count;
}

// static
void SearchTest::Cleanup(platform::LocalCountryFile const & map)
{
  platform::CountryIndexes::DeleteFromDisk(map);
  map.DeleteFromDisk(MapOptions::Map);
}
}  // namespace search
