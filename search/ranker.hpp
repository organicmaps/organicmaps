#pragma once

#include "search/cancel_exception.hpp"
#include "search/emitter.hpp"
#include "search/geocoder.hpp"
#include "search/intermediate_result.hpp"
#include "search/keyword_lang_matcher.hpp"
#include "search/locality_finder.hpp"
#include "search/region_info_getter.hpp"
#include "search/result.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/suggest.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <string>
#include <vector>

class CategoriesHolder;
class DataSource;

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace search
{
class CitiesBoundariesTable;
class RankerResultMaker;
class VillagesCache;

class Ranker
{
public:
  struct Params
  {
    m2::RectD m_viewport;
    m2::PointD m_pivot;
    std::string m_pivotRegion;
    std::vector<uint32_t> m_preferredTypes;
    bool m_suggestsEnabled = false;
    bool m_needAddress = false;
    bool m_needHighlighting = false;
    bool m_viewportSearch = false;
    bool m_categorialRequest = false;

    std::string m_query;
    QueryTokens m_tokens;
    // Prefix of the last token in the query.
    // We need it here to make suggestions.
    strings::UniString m_prefix;

    // Minimal distance between search results in meters, needed for
    // filtering of identical search results.
    double m_minDistanceBetweenResultsM = 100.0;

    Locales m_categoryLocales;

    // The maximum number of results in a single emit.
    size_t m_batchSize = 0;

    // The maximum total number of results to be emitted in all batches.
    size_t m_limit = 0;
  };

  Ranker(DataSource const & dataSource, CitiesBoundariesTable const & boundariesTable,
         storage::CountryInfoGetter const & infoGetter, KeywordLangMatcher & keywordsScorer,
         Emitter & emitter, CategoriesHolder const & categories,
         std::vector<Suggest> const & suggests, VillagesCache & villagesCache,
         base::Cancellable const & cancellable);
  virtual ~Ranker() = default;

  void Init(Params const & params, Geocoder::Params const & geocoderParams);

  void Finish(bool cancelled);

  bool IsFull() const { return m_emitter.GetResults().GetCount() >= m_params.m_limit; }

  // Makes the final result that is shown to the user from a ranker's result.
  // |needAddress| and |needHighlighting| enable filling of optional fields
  // that may take a considerable amount of time to compute.
  Result MakeResult(RankerResult rankerResult, bool needAddress, bool needHighlighting) const;

  void SuggestStrings();

  virtual void AddPreRankerResults(std::vector<PreRankerResult> && preRankerResults)
  {
    std::move(preRankerResults.begin(), preRankerResults.end(),
              std::back_inserter(m_preRankerResults));
  }

  virtual void UpdateResults(bool lastUpdate);

  void ClearCaches();

  void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

  void SetLocale(std::string const & locale);

  void LoadCountriesTree();

private:
  friend class RankerResultMaker;

  void MakeRankerResults();

  void GetBestMatchName(FeatureType & f, std::string & name) const;
  void MatchForSuggestions(strings::UniString const & token, int8_t locale,
                           std::string const & prolog);
  void ProcessSuggestions(std::vector<RankerResult> const & vec) const;

  std::string GetLocalizedRegionInfoForResult(RankerResult const & result) const;

  Params m_params;
  Geocoder::Params m_geocoderParams;
  ReverseGeocoder const m_reverseGeocoder;
  base::Cancellable const & m_cancellable;
  KeywordLangMatcher & m_keywordsScorer;

  mutable LocalityFinder m_localities;
  RegionInfoGetter m_regionInfoGetter;

  DataSource const & m_dataSource;
  storage::CountryInfoGetter const & m_infoGetter;
  Emitter & m_emitter;
  CategoriesHolder const & m_categories;
  std::vector<Suggest> const & m_suggests;

  std::vector<PreRankerResult> m_preRankerResults;
  std::vector<RankerResult> m_tentativeResults;
};
}  // namespace search
