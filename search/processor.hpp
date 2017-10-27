#pragma once
#include "search/categories_cache.hpp"
#include "search/categories_set.hpp"
#include "search/cities_boundaries_table.hpp"
#include "search/emitter.hpp"
#include "search/geocoder.hpp"
#include "search/hotels_filter.hpp"
#include "search/mode.hpp"
#include "search/pre_ranker.hpp"
#include "search/rank_table_cache.hpp"
#include "search/ranker.hpp"
#include "search/search_params.hpp"
#include "search/search_trie.hpp"
#include "search/suggest.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/rank_table.hpp"
#include "indexer/string_slice.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/limited_priority_queue.hpp"
#include "base/string_utils.hpp"

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_set.hpp"
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

  Processor(Index const & index, CategoriesHolder const & categories,
            vector<Suggest> const & suggests, storage::CountryInfoGetter const & infoGetter);

  void Init(bool viewportSearch);

  void SetViewport(m2::RectD const & viewport);
  void SetPreferredLocale(string const & locale);
  void SetInputLocale(string const & locale);
  void SetQuery(string const & query);
  // TODO (@y): this function must be removed.
  void SetRankPivot(m2::PointD const & pivot);
  inline void SetMode(Mode mode) { m_mode = mode; }
  inline void SetSuggestsEnabled(bool enabled) { m_suggestsEnabled = enabled; }
  inline void SetPosition(m2::PointD const & position) { m_position = position; }
  inline void SetMinDistanceOnMapBetweenResults(double distance)
  {
    m_minDistanceOnMapBetweenResults = distance;
  }
  inline void SetOnResults(SearchParams::OnResults const & onResults) { m_onResults = onResults; }
  inline string const & GetPivotRegion() const { return m_region; }
  inline m2::PointD const & GetPosition() const { return m_position; }

  /// Suggestions language code, not the same as we use in mwm data
  int8_t m_inputLocaleCode;
  int8_t m_currentLocaleCode;

  inline bool IsEmptyQuery() const { return (m_prefix.empty() && m_tokens.empty()); }

  void Search(SearchParams const & params);

  // Tries to generate a (lat, lon) result from |m_query|.
  void SearchCoordinates();

  void InitParams(QueryParams & params);

  void InitGeocoder(Geocoder::Params & params);
  void InitPreRanker(Geocoder::Params const & geocoderParams);
  void InitRanker(Geocoder::Params const & geocoderParams);
  void InitEmitter();

  void ClearCaches();
  void LoadCitiesBoundaries();

protected:
  using TMWMVector = vector<shared_ptr<MwmInfo>>;
  using TOffsetsVector = map<MwmSet::MwmId, vector<uint32_t>>;
  using TFHeader = feature::DataHeader;

  Locales GetCategoryLocales() const;

  template <typename ToDo>
  void ForEachCategoryType(StringSliceBase const & slice, ToDo && toDo) const;

  template <typename ToDo>
  void ForEachCategoryTypeFuzzy(StringSliceBase const & slice, ToDo && toDo) const;

  m2::PointD GetPivotPoint() const;
  m2::RectD GetPivotRect() const;

  m2::RectD const & GetViewport() const;

  void SetLanguage(int id, int8_t lang);
  int8_t GetLanguage(int id) const;

  CategoriesHolder const & m_categories;
  storage::CountryInfoGetter const & m_infoGetter;

  string m_region;
  string m_query;
  QueryTokens m_tokens;
  strings::UniString m_prefix;
  set<uint32_t> m_preferredTypes;

  m2::RectD m_viewport;
  m2::PointD m_pivot;
  m2::PointD m_position;
  double m_minDistanceOnMapBetweenResults;
  Mode m_mode;
  bool m_suggestsEnabled;
  shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  bool m_cianMode = false;
  SearchParams::OnResults m_onResults;

  bool m_viewportSearch;

  VillagesCache m_villagesCache;
  CitiesBoundariesTable m_citiesBoundaries;

  KeywordLangMatcher m_keywordsScorer;
  Emitter m_emitter;
  Ranker m_ranker;
  PreRanker m_preRanker;
  Geocoder m_geocoder;
};
}  // namespace search
