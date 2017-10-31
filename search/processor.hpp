#pragma once
#include "search/categories_cache.hpp"
#include "search/categories_set.hpp"
#include "search/cities_boundaries_table.hpp"
#include "search/emitter.hpp"
#include "search/geocoder.hpp"
#include "search/pre_ranker.hpp"
#include "search/rank_table_cache.hpp"
#include "search/ranker.hpp"
#include "search/search_params.hpp"
#include "search/search_trie.hpp"
#include "search/suggest.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "indexer/index.hpp"
#include "indexer/string_slice.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

class FeatureType;
class CategoriesHolder;

namespace coding
{
class CompressedBitVector;
}

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
class Geocoder;
class QueryParams;
class Ranker;
class ReverseGeocoder;

class Processor : public my::Cancellable
{
public:
  // Maximum result candidates count for each viewport/criteria.
  static size_t const kPreResultsCount;

  static double const kMinViewportRadiusM;
  static double const kMaxViewportRadiusM;

  static double const kMinDistanceOnMapBetweenResultsM;

  Processor(Index const & index, CategoriesHolder const & categories,
            vector<Suggest> const & suggests, storage::CountryInfoGetter const & infoGetter);

  void SetViewport(m2::RectD const & viewport);
  void SetPreferredLocale(string const & locale);
  void SetInputLocale(string const & locale);
  void SetQuery(string const & query);
  inline void SetPosition(m2::PointD const & position) { m_position = position; }
  inline string const & GetPivotRegion() const { return m_region; }
  inline m2::PointD const & GetPosition() const { return m_position; }

  inline bool IsEmptyQuery() const { return m_prefix.empty() && m_tokens.empty(); }

  void Search(SearchParams const & params);

  // Tries to generate a (lat, lon) result from |m_query|.
  void SearchCoordinates();

  void InitParams(QueryParams & params);

  void InitGeocoder(Geocoder::Params &geocoderParams,
                    SearchParams const &searchParams);
  void InitPreRanker(Geocoder::Params const &geocoderParams,
                     SearchParams const &searchParams);
  void InitRanker(Geocoder::Params const & geocoderParams, SearchParams const & searchParams);
  void InitEmitter(SearchParams const & searchParams);

  void ClearCaches();
  void LoadCitiesBoundaries();

protected:
  Locales GetCategoryLocales() const;

  template <typename ToDo>
  void ForEachCategoryType(StringSliceBase const & slice, ToDo && toDo) const;

  template <typename ToDo>
  void ForEachCategoryTypeFuzzy(StringSliceBase const & slice, ToDo && toDo) const;

  m2::PointD GetPivotPoint(bool viewportSearch) const;
  m2::RectD GetPivotRect(bool viewportSearch) const;

  m2::RectD const & GetViewport() const;

  CategoriesHolder const & m_categories;
  storage::CountryInfoGetter const & m_infoGetter;

  string m_region;
  string m_query;
  QueryTokens m_tokens;
  strings::UniString m_prefix;
  set<uint32_t> m_preferredTypes;

  m2::RectD m_viewport;
  m2::PointD m_position;
  bool m_needAddress = true;
  bool m_needHighlight = true;

  // Suggestions language code, not the same as we use in mwm data
  int8_t m_inputLocaleCode = StringUtf8Multilang::kUnsupportedLanguageCode;
  int8_t m_currentLocaleCode = StringUtf8Multilang::kUnsupportedLanguageCode;

  VillagesCache m_villagesCache;
  CitiesBoundariesTable m_citiesBoundaries;

  KeywordLangMatcher m_keywordsScorer;
  Emitter m_emitter;
  Ranker m_ranker;
  PreRanker m_preRanker;
  Geocoder m_geocoder;
};
}  // namespace search
