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
class PreResult2Maker;
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
    bool m_viewportSearch = false;

    string m_query;
    buffer_vector<strings::UniString, 32> m_tokens;
    // Prefix of the last token in the query.
    // We need it here to make suggestions.
    strings::UniString m_prefix;

    m2::PointD m_accuratePivotCenter = m2::PointD(0, 0);

    // A minimum distance between search results in meters, needed for
    // filtering of indentical search results.
    double m_minDistanceOnMapBetweenResults = 0.0;

    Locales m_categoryLocales;

    size_t m_limit = 0;
  };

  static size_t const kBatchSize;

  Ranker(Index const & index, CitiesBoundariesTable const & boundariesTable,
         storage::CountryInfoGetter const & infoGetter, Emitter & emitter,
         CategoriesHolder const & categories, vector<Suggest> const & suggests,
         VillagesCache & villagesCache, my::Cancellable const & cancellable);
  virtual ~Ranker() = default;

  void Init(Params const & params, Geocoder::Params const & geocoderParams);

  bool IsResultExists(PreResult2 const & p, vector<IndexedValue> const & values);

  void MakePreResult2(Geocoder::Params const & params, vector<IndexedValue> & cont);

  Result MakeResult(PreResult2 const & r) const;
  void MakeResultHighlight(Result & res) const;

  void GetSuggestion(string const & name, string & suggest) const;
  void SuggestStrings();
  void MatchForSuggestions(strings::UniString const & token, int8_t locale, string const & prolog);
  void GetBestMatchName(FeatureType const & f, string & name) const;
  void ProcessSuggestions(vector<IndexedValue> & vec) const;

  virtual void SetPreResults1(vector<PreResult1> && preResults1) { m_preResults1 = move(preResults1); }
  virtual void UpdateResults(bool lastUpdate);

  void ClearCaches();

  inline void SetLocalityLanguage(int8_t code) { m_localityLang = code; }

  inline void SetLanguage(pair<int, int> const & ind, int8_t lang)
  {
    m_keywordsScorer.SetLanguage(ind, lang);
  }

  inline int8_t GetLanguage(pair<int, int> const & ind) const
  {
    return m_keywordsScorer.GetLanguage(ind);
  }

  inline void SetLanguages(vector<vector<int8_t>> const & languagePriorities)
  {
    m_keywordsScorer.SetLanguages(languagePriorities);
  }

  inline void SetKeywords(KeywordMatcher::StringT const * keywords, size_t count,
                          KeywordMatcher::StringT const & prefix)
  {
    m_keywordsScorer.SetKeywords(keywords, count, prefix);
  }

  inline void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

private:
  friend class PreResult2Maker;

  Params m_params;
  Geocoder::Params m_geocoderParams;
  ReverseGeocoder const m_reverseGeocoder;
  my::Cancellable const & m_cancellable;
  KeywordLangMatcher m_keywordsScorer;

  mutable LocalityFinder m_localities;
  int8_t m_localityLang = StringUtf8Multilang::kDefaultCode;

  Index const & m_index;
  storage::CountryInfoGetter const & m_infoGetter;
  Emitter & m_emitter;
  CategoriesHolder const & m_categories;
  vector<Suggest> const & m_suggests;

  vector<PreResult1> m_preResults1;
  vector<IndexedValue> m_tentativeResults;
};
}  // namespace search
