#pragma once

#include "search/cancel_exception.hpp"
#include "search/geocoder.hpp"
#include "search/intermediate_result.hpp"
#include "search/keyword_lang_matcher.hpp"
#include "search/locality_finder.hpp"
#include "search/mode.hpp"
#include "search/result.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/search_params.hpp"
#include "search/suggest.hpp"
#include "search/utils.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class CategoriesHolder;
class Index;

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace search
{
class CitiesBoundariesTable;
class Emitter;
class RankerResultMaker;
class VillagesCache;

class Ranker
{
public:
  struct Params
  {
    int8_t m_currentLocaleCode = CategoriesHolder::kEnglishCode;
    m2::RectD m_viewport;
    m2::PointD m_position;
    string m_pivotRegion;
    set<uint32_t> m_preferredTypes;
    bool m_suggestsEnabled = false;
    bool m_needAddress = false;
    bool m_needHighlight = false;
    bool m_viewportSearch = false;

    string m_query;
    QueryTokens m_tokens;
    // Prefix of the last token in the query.
    // We need it here to make suggestions.
    strings::UniString m_prefix;

    m2::PointD m_accuratePivotCenter = m2::PointD(0, 0);

    // A minimum distance between search results in meters, needed for
    // filtering of identical search results.
    double m_minDistanceOnMapBetweenResults = 0.0;

    Locales m_categoryLocales;

    // Default batch size. Override if needed.
    size_t m_batchSize = 10;

    // The maximum total number of results to be emitted in all batches.
    size_t m_limit = 0;
  };

  Ranker(Index const & index, CitiesBoundariesTable const & boundariesTable,
         storage::CountryInfoGetter const & infoGetter, KeywordLangMatcher & keywordsScorer,
         Emitter & emitter, CategoriesHolder const & categories, vector<Suggest> const & suggests,
         VillagesCache & villagesCache, my::Cancellable const & cancellable);
  virtual ~Ranker() = default;

  void Init(Params const & params, Geocoder::Params const & geocoderParams);

  // Makes the final result that is shown to the user from a ranker's result.
  // |needAddress| and |needHighlighting| enable filling of optional fields
  // that may take a considerable amount of time to compute.
  Result MakeResult(RankerResult const & r, bool needAddress, bool needHighlighting) const;

  void SuggestStrings();

  virtual void SetPreRankerResults(vector<PreRankerResult> && preRankerResults)
  {
    m_preRankerResults = move(preRankerResults);
  }
  virtual void UpdateResults(bool lastUpdate);

  void ClearCaches();

  inline void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

  inline void SetLocalityLanguage(int8_t code) { m_localityLang = code; }

private:
  friend class RankerResultMaker;

  void MakeRankerResults(Geocoder::Params const & params, vector<RankerResult> & results);

  void GetBestMatchName(FeatureType const & f, string & name) const;
  void MatchForSuggestions(strings::UniString const & token, int8_t locale, string const & prolog);
  void ProcessSuggestions(vector<RankerResult> & vec) const;

  Params m_params;
  Geocoder::Params m_geocoderParams;
  ReverseGeocoder const m_reverseGeocoder;
  my::Cancellable const & m_cancellable;
  KeywordLangMatcher & m_keywordsScorer;

  mutable LocalityFinder m_localities;
  int8_t m_localityLang = StringUtf8Multilang::kDefaultCode;

  Index const & m_index;
  storage::CountryInfoGetter const & m_infoGetter;
  Emitter & m_emitter;
  CategoriesHolder const & m_categories;
  vector<Suggest> const & m_suggests;

  vector<PreRankerResult> m_preRankerResults;
  vector<RankerResult> m_tentativeResults;
};
}  // namespace search
