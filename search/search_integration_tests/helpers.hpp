#pragma once

#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/search_tests_support/test_with_custom_mwms.hpp"

#include "indexer/indexer_tests_support/helpers.hpp"

#include "geometry/rect2d.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace search
{
class SearchTest : public tests_support::TestWithCustomMwms
{
public:
  using TRule = std::shared_ptr<tests_support::MatchingRule>;
  using TRules = std::vector<TRule>;

  SearchTest();
  ~SearchTest() override = default;

  // Registers country in internal records. Note that physical country
  // file may be absent.
  void RegisterCountry(std::string const & name, m2::RectD const & rect);

  inline void SetViewport(m2::RectD const & viewport) { m_viewport = viewport; }

  bool ResultsMatch(std::string const & query, TRules const & rules);

  bool ResultsMatch(std::string const & query, string const & locale, TRules const & rules);

  bool ResultsMatch(std::string const & query, Mode mode, TRules const & rules);

  bool ResultsMatch(std::vector<search::Result> const & results, TRules const & rules);

  bool ResultsMatch(SearchParams const & params, TRules const & rules);

  bool ResultMatches(search::Result const & result, TRule const & rule);

  std::unique_ptr<tests_support::TestSearchRequest> MakeRequest(std::string const & query,
                                                                std::string const & locale = "en");

  size_t CountFeatures(m2::RectD const & rect);

protected:
  void OnMwmBuilt(MwmInfo const & /* info */) override;

  my::ScopedLogLevelChanger m_scopedLog;

  tests_support::TestSearchEngine m_engine;

  m2::RectD m_viewport;
};
}  // namespace search
