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
  SetViewport(mercator::Bounds::FullRect());
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

bool SearchTest::ResultsMatch(vector<search::Result> const & results, Rules const & rules)
{
  return MatchResults(m_dataSource, rules, results);
}

bool SearchTest::ResultsMatch(SearchParams const & params, Rules const & rules)
{
  tests_support::TestSearchRequest request(m_engine, params);
  request.Run();
  return ResultsMatch(request.Results(), rules);
}

bool SearchTest::ResultMatches(search::Result const & result, Rule const & rule)
{
  return tests_support::ResultMatches(m_dataSource, rule, result);
}

bool SearchTest::AlternativeMatch(string const & query, vector<Rules> const & rulesList)
{
  tests_support::TestSearchRequest request(m_engine, query, "en", Mode::Everywhere, m_viewport);
  request.Run();
  return tests_support::AlternativeMatch(m_dataSource, rulesList, request.Results());
}

size_t SearchTest::GetResultsNumber(string const & query, string const & locale)
{
  tests_support::TestSearchRequest request(m_engine, query, locale, Mode::Everywhere, m_viewport);
  request.Run();
  return request.Results().size();
}

unique_ptr<tests_support::TestSearchRequest> SearchTest::MakeRequest(SearchParams params)
{
  auto request = make_unique<tests_support::TestSearchRequest>(m_engine, params);
  request->Run();
  return request;
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
  params.m_streetSearchRadiusM = tests_support::TestSearchRequest::kDefaultTestStreetSearchRadiusM;
  params.m_villageSearchRadiusM =
      tests_support::TestSearchRequest::kDefaultTestVillageSearchRadiusM;

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
