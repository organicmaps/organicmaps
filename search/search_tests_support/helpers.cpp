#include "search/search_tests_support/helpers.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

namespace search
{
namespace tests_support
{
using namespace std;

SearchTestBase::SearchTestBase(base::LogLevel logLevel, bool mockCountryInfo)
  : m_scopedLog(logLevel)
  , m_engine(m_dataSource, {}, mockCountryInfo)
{
  SetViewport(mercator::Bounds::FullRect());

  if (!mockCountryInfo)
    m_engine.InitAffiliations();
}

void SearchTestBase::SetViewport(ms::LatLon const & ll, double radiusM)
{
  SetViewport(mercator::MetersToXY(ll.m_lon, ll.m_lat, radiusM));
}

bool SearchTestBase::CategoryMatch(std::string const & query, Rules const & rules, string const & locale /* = "en" */)
{
  TestSearchRequest request(m_engine, query, locale, Mode::Everywhere, m_viewport);
  request.SetCategorial();

  request.Run();
  return MatchResults(m_dataSource, rules, request.Results());
}

bool SearchTestBase::ResultsMatch(std::string const & query, Rules const & rules,
                                  std::string const & locale /* = "en" */, Mode mode /* = Mode::Everywhere */)
{
  TestSearchRequest request(m_engine, query, locale, mode, m_viewport);
  request.Run();
  return MatchResults(m_dataSource, rules, request.Results());
}

bool SearchTestBase::OrderedResultsMatch(std::string const & query, Rules const & rules,
                                         std::string const & locale /* = "en" */, Mode mode /* = Mode::Everywhere */)
{
  TestSearchRequest request(m_engine, query, locale, mode, m_viewport);
  request.Run();
  return OrderedResultsMatch(request.Results(), rules);
}

bool SearchTestBase::ResultsMatch(vector<search::Result> const & results, Rules const & rules)
{
  return MatchResults(m_dataSource, rules, results);
}

bool SearchTestBase::OrderedResultsMatch(std::vector<Result> const & results, Rules const & rules)
{
  if (results.size() != rules.size())
  {
    LOG(LWARNING, ("Unexpected results number:", results));
    return false;
  }

  for (size_t i = 0; i < results.size(); ++i)
  {
    if (!ResultMatches(m_dataSource, rules[i], results[i]))
    {
      LOG(LWARNING, ("Not matched:", rules[i], results[i]));
      return false;
    }
  }
  return true;
}

bool SearchTestBase::IsResultMatches(search::Result const & result, Rule const & rule)
{
  return tests_support::ResultMatches(m_dataSource, rule, result);
}

bool SearchTestBase::AlternativeMatch(string const & query, vector<Rules> const & rulesList)
{
  TestSearchRequest request(m_engine, query, "en", Mode::Everywhere, m_viewport);
  request.Run();
  return tests_support::AlternativeMatch(m_dataSource, rulesList, request.Results());
}

size_t SearchTestBase::GetResultsNumber(string const & query, string const & locale)
{
  TestSearchRequest request(m_engine, query, locale, Mode::Everywhere, m_viewport);
  request.Run();
  return request.Results().size();
}

SearchParams SearchTestBase::GetDefaultSearchParams(string const & query, string const & locale /* = "en" */) const
{
  SearchParams params;
  params.m_query = query;
  params.m_inputLocale = locale;
  params.m_viewport = m_viewport;
  params.m_mode = Mode::Everywhere;
  params.m_needAddress = true;
  params.m_suggestsEnabled = false;
  return params;
}

unique_ptr<TestSearchRequest> SearchTestBase::MakeRequest(SearchParams const & params)
{
  auto request = make_unique<TestSearchRequest>(m_engine, params);
  request->Run();
  return request;
}

size_t SearchTestBase::CountFeatures(m2::RectD const & rect)
{
  size_t count = 0;
  auto counter = [&count](FeatureType const & /* ft */) { ++count; };
  m_dataSource.ForEachInRect(counter, rect, scales::GetUpperScale());
  return count;
}

void SearchTest::RegisterCountry(string const & name, m2::RectD const & rect)
{
  auto & infoGetter = dynamic_cast<storage::CountryInfoGetterForTesting &>(m_engine.GetCountryInfoGetter());
  infoGetter.AddCountry(storage::CountryDef(name, rect));
}

void SearchTest::OnMwmBuilt(MwmInfo const & info)
{
  switch (info.GetType())
  {
  case MwmInfo::COUNTRY: RegisterCountry(info.GetCountryName(), info.m_bordersRect); break;
  case MwmInfo::WORLD: m_engine.LoadCitiesBoundaries(); break;
  case MwmInfo::COASTS: break;
  }
}

}  // namespace tests_support
}  // namespace search
