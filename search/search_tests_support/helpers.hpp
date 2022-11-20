#pragma once

#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/search_tests_support/test_with_custom_mwms.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "geometry/rect2d.hpp"

#include <memory>
#include <string>
#include <vector>

namespace search
{
namespace tests_support
{

class SearchTestBase : public TestWithCustomMwms
{
public:
  using Rule = std::shared_ptr<MatchingRule>;
  using Rules = std::vector<Rule>;

  SearchTestBase(base::LogLevel logLevel, bool mockCountryInfo);

  inline void SetViewport(m2::RectD const & viewport) { m_viewport = viewport; }
  void SetViewport(ms::LatLon const & ll, double radiusM);

  bool CategoryMatch(std::string const & query, Rules const & rules,
                     std::string const & locale = "en");

  bool ResultsMatch(std::string const & query, Rules const & rules,
                    std::string const & locale = "en",
                    Mode mode = Mode::Everywhere);
  bool ResultsMatch(std::vector<Result> const & results, Rules const & rules);
  bool ResultsMatch(SearchParams const & params, Rules const & rules);

  bool IsResultMatches(Result const & result, Rule const & rule);

  bool AlternativeMatch(std::string const & query, std::vector<Rules> const & rulesList);

  size_t GetResultsNumber(std::string const & query, std::string const & locale);

  std::unique_ptr<TestSearchRequest> MakeRequest(SearchParams const & params);
  std::unique_ptr<TestSearchRequest> MakeRequest(std::string const & query, std::string const & locale = "en");

  size_t CountFeatures(m2::RectD const & rect);

protected:
  base::ScopedLogLevelChanger m_scopedLog;

  TestSearchEngine m_engine;

  m2::RectD m_viewport;
};

class SearchTest : public SearchTestBase
{
public:
  explicit SearchTest(base::LogLevel logLevel = base::LDEBUG)
    : SearchTestBase(logLevel, true /* mockCountryInfo*/)
  {
  }

  // Registers country in internal records. Note that physical country file may be absent.
  void RegisterCountry(std::string const & name, m2::RectD const & rect);

protected:
  void OnMwmBuilt(MwmInfo const & /* info */) override;
};

class TestCafe : public generator::tests_support::TestPOI
{
public:
  TestCafe(m2::PointD const & center, std::string const & name, std::string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"amenity", "cafe"}});
  }

  explicit TestCafe(m2::PointD const & center) : TestCafe(center, "cafe", "en") {}
};

class TestHotel : public generator::tests_support::TestPOI
{
public:
  TestHotel(m2::PointD const & center, std::string const & name, std::string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"tourism", "hotel"}});
  }

  explicit TestHotel(m2::PointD const & center) : TestHotel(center, "hotel", "en") {}
};

} // namespace tests_support
} // namespace search
