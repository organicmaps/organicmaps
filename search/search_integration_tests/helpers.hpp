#pragma once

#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "storage/country_decl.hpp"
#include "storage/country_info_getter.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"
#include "indexer/indexer_tests_support/helpers.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"

#include "base/assert.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Platform;

namespace search
{
class TestWithClassificator
{
public:
  TestWithClassificator();
};

class SearchTest : public TestWithClassificator
{
public:
  using TRule = shared_ptr<tests_support::MatchingRule>;
  using TRules = vector<TRule>;

  SearchTest();

  ~SearchTest();

  // Registers country in internal records. Note that physical country
  // file may be absent.
  void RegisterCountry(string const & name, m2::RectD const & rect);

  // Creates a physical country file on a disk, which will be removed
  // at the end of the test. |fn| is a delegate that accepts a single
  // argument - TestMwmBuilder and adds all necessary features to the
  // country file.
  //
  // *NOTE* when |type| is feature::DataHeader::country, the country
  // with |name| will be automatically registered.
  template <typename TBuildFn>
  MwmSet::MwmId BuildMwm(string const & name, feature::DataHeader::MapType type, TBuildFn && fn)
  {
    m_files.emplace_back(m_platform.WritableDir(), platform::CountryFile(name), 0 /* version */);
    auto & file = m_files.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, type);
      fn(builder);
    }

    auto result = m_engine.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const & id = result.first;
    if (type == feature::DataHeader::country)
    {
      if (auto const & info = id.GetInfo())
        RegisterCountry(name, info->m_limitRect);
    }
    return id;
  }

  template <typename TBuildFn>
  MwmSet::MwmId BuildWorld(TBuildFn && fn)
  {
    auto const id = BuildMwm("testWorld", feature::DataHeader::world, forward<TBuildFn>(fn));
    m_engine.LoadCitiesBoundaries();
    return id;
  }

  template <typename TBuildFn>
  MwmSet::MwmId BuildCountry(string const & name, TBuildFn && fn)
  {
    return BuildMwm(name, feature::DataHeader::country, forward<TBuildFn>(fn));
  }

  template <typename TEditorFn>
  void EditFeature(FeatureID const & id, TEditorFn && fn)
  {
    Index::FeaturesLoaderGuard loader(m_engine, id.m_mwmId);
    FeatureType ft;
    CHECK(loader.GetFeatureByIndex(id.m_index, ft), ());
    indexer::tests_support::EditFeature(ft, forward<TEditorFn>(fn));
  }

  inline void SetViewport(m2::RectD const & viewport) { m_viewport = viewport; }

  bool ResultsMatch(string const & query, TRules const & rules);

  bool ResultsMatch(string const & query, string const & locale, TRules const & rules);

  bool ResultsMatch(string const & query, Mode mode, TRules const & rules);

  bool ResultsMatch(vector<search::Result> const & results, TRules const & rules);

  bool ResultsMatch(SearchParams const & params, TRules const & rules);

  bool ResultMatches(search::Result const & result, TRule const & rule);

  unique_ptr<tests_support::TestSearchRequest> MakeRequest(string const & query,
                                                           string const & locale = "en");

  size_t CountFeatures(m2::RectD const & rect);

protected:
  static void Cleanup(platform::LocalCountryFile const & map);

  Platform & m_platform;
  my::ScopedLogLevelChanger m_scopedLog;
  vector<platform::LocalCountryFile> m_files;
  tests_support::TestSearchEngine m_engine;
  m2::RectD m_viewport;
};
}  // namespace search
