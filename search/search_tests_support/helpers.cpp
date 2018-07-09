#include "search/search_tests_support/helpers.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

using namespace std;

namespace search
{
SearchTest::SearchTest()
  : m_scopedLog(LDEBUG)
  , m_engine(m_dataSource, make_unique<storage::CountryInfoGetterForTesting>(), Engine::Params{})
{
  SetViewport(MercatorBounds::FullRect());
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

bool SearchTest::ResultsMatch(string const & query, string const & locale,
                              vector<shared_ptr<tests_support::MatchingRule>> const & rules)
{
  tests_support::TestSearchRequest request(m_engine, query, locale, Mode::Everywhere, m_viewport);
  request.Run();
  return MatchResults(m_dataSource, rules, request.Results());
}

bool SearchTest::ResultsMatch(string const & query, Mode mode,
                              vector<shared_ptr<tests_support::MatchingRule>> const & rules)
{
  tests_support::TestSearchRequest request(m_engine, query, "en", mode, m_viewport);
  request.Run();
  return MatchResults(m_dataSource, rules, request.Results());
}

bool SearchTest::ResultsMatch(vector<search::Result> const & results, TRules const & rules)
{
  return MatchResults(m_dataSource, rules, results);
}

bool SearchTest::ResultsMatch(SearchParams const & params, TRules const & rules)
{
  tests_support::TestSearchRequest request(m_engine, params);
  request.Run();
  return ResultsMatch(request.Results(), rules);
}

bool SearchTest::ResultMatches(search::Result const & result, TRule const & rule)
{
  return tests_support::ResultMatches(m_dataSource, rule, result);
}

unique_ptr<tests_support::TestSearchRequest> SearchTest::MakeRequest(
    string const & query, string const & locale /* = "en" */)
{
  SearchParams params;
  params.m_query = query;
  params.m_inputLocale = locale;
  params.m_viewport = m_viewport;
  params.m_mode = Mode::Everywhere;
  params.m_needAddress = true;
  params.m_suggestsEnabled = false;

  auto request = make_unique<tests_support::TestSearchRequest>(m_engine, params);
  request->Run();
  return request;
}

size_t SearchTest::CountFeatures(m2::RectD const & rect)
{
  size_t count = 0;
  auto counter = [&count](const FeatureType & /* ft */) { ++count; };
  m_dataSource.ForEachInRect(counter, rect, scales::GetUpperScale());
  return count;
}

// static
void SearchTest::OnMwmBuilt(MwmInfo const & info)
{
  switch (info.GetType())
  {
  case MwmInfo::COUNTRY: RegisterCountry(info.GetCountryName(), info.m_bordersRect); break;
  case MwmInfo::WORLD: m_engine.LoadCitiesBoundaries(); break;
  case MwmInfo::COASTS: break;
  }
}
}  // namespace search
