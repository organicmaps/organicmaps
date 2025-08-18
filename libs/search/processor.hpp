#pragma once

#include "search/bookmarks/processor.hpp"
#include "search/bookmarks/types.hpp"
#include "search/cities_boundaries_table.hpp"
#include "search/common.hpp"
#include "search/emitter.hpp"
#include "search/geocoder.hpp"
#include "search/pre_ranker.hpp"
#include "search/ranker.hpp"
#include "search/search_params.hpp"
#include "search/suggest.hpp"

#include "ge0/geo_url_parser.hpp"

#include "indexer/string_slice.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/mem_trie.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class FeatureType;
class CategoriesHolder;
class DataSource;
class MwmInfo;

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

class Processor : public base::Cancellable
{
public:
  // Maximum result candidates count for each viewport/criteria.
  static size_t const kPreResultsCount;

  Processor(DataSource const & dataSource, CategoriesHolder const & categories, std::vector<Suggest> const & suggests,
            storage::CountryInfoGetter const & infoGetter);

  void SetViewport(m2::RectD const & viewport);
  void SetPreferredLocale(std::string const & locale);
  void SetInputLocale(std::string const & locale);
  void SetQuery(std::string const & query, bool categorialRequest = false);

  inline bool IsEmptyQuery() const { return m_query.IsEmpty(); }

  void Search(SearchParams params);

  /// Tries to parse a custom debugging command from |m_query|.
  /// @return True if can stop further search.
  bool SearchDebug();
  // Tries to generate a (lat, lon) result from |m_query|.
  // Returns true if |m_query| contains coordinates.
  bool SearchCoordinates();
  // Tries to parse a plus code from |m_query| and generate a (lat, lon) result.
  void SearchPlusCode();
  // Tries to parse a postcode from |m_query| and generate a (lat, lon) result based on
  // POSTCODE_POINTS section.
  void SearchPostcode();

  void SearchBookmarks(bookmarks::GroupId const & groupId);

  void InitParams(QueryParams & params) const;

  void InitGeocoder(Geocoder::Params & geocoderParams, SearchParams const & searchParams);
  void InitPreRanker(Geocoder::Params const & geocoderParams, SearchParams const & searchParams);
  void InitRanker(Geocoder::Params const & geocoderParams, SearchParams const & searchParams);

  void ClearCaches();
  void CacheWorldLocalities();
  void LoadCitiesBoundaries();
  void LoadCountriesTree();

  void EnableIndexingOfBookmarksDescriptions(bool enable);
  void EnableIndexingOfBookmarkGroup(bookmarks::GroupId const & groupId, bool enable);

  void ResetBookmarks();

  void OnBookmarksCreated(std::vector<std::pair<bookmarks::Id, bookmarks::Doc>> const & marks);
  void OnBookmarksUpdated(std::vector<std::pair<bookmarks::Id, bookmarks::Doc>> const & marks);
  void OnBookmarksDeleted(std::vector<bookmarks::Id> const & marks);
  void OnBookmarksAttachedToGroup(bookmarks::GroupId const & groupId, std::vector<bookmarks::Id> const & marks);
  void OnBookmarksDetachedFromGroup(bookmarks::GroupId const & groupId, std::vector<bookmarks::Id> const & marks);

  // base::Cancellable overrides:
  void Reset() override;
  bool IsCancelled() const override;

protected:
  // Show feature by FeatureId. May try to guess as much as possible after the "fid=" prefix but
  // at least supports the formats below.
  // 0. fid=123 or ?fid=123 to search for the feature with index 123, results ordered by distance
  //    from |m_position| or |m_viewport|, whichever is present and closer.
  // 1. fid=MwmName,123 or fid=(MwmName,123) to search for the feature with
  //    index 123 in the Mwm "MwmName" (for example, "Laos" or "Laos.mwm").
  // 2. fid={ MwmId [Laos, 200623], 123 } or just { MwmId [Laos, 200623], 123 } or whatever current
  //    format of the string returned by FeatureID's DebugPrint is.
  void SearchByFeatureId();
  void EmitWithMetadata(feature::Metadata::EType type);

  Locales GetCategoryLocales() const;

  template <typename ToDo>
  void ForEachCategoryType(StringSliceBase const & slice, ToDo && toDo) const;

  template <typename ToDo>
  void ForEachCategoryTypeFuzzy(StringSliceBase const & slice, ToDo && toDo) const;

  m2::PointD GetPivotPoint(bool viewportSearch) const;
  m2::RectD GetPivotRect(bool viewportSearch) const;

  m2::RectD const & GetViewport() const;

  // The results are sorted by distance (to a point from |m_viewport| or |m_position|) before being emitted.
  template <class FnT>
  void EmitResultsFromMwms(std::vector<std::shared_ptr<MwmInfo>> const & infos, FnT const & fn);

  CategoriesHolder const & m_categories;
  storage::CountryInfoGetter const & m_infoGetter;
  using CountriesTrie = base::MemTrie<storage::CountryId, base::VectorValues<bool>>;
  CountriesTrie m_countriesTrie;

  /// @todo Replace with QueryParams.
  /// @{
  QueryString m_query;
  bool m_isCategorialRequest;
  /// @}

  std::vector<uint32_t> m_preferredTypes;
  std::vector<uint32_t> m_cuisineTypes;

  m2::RectD m_viewport;
  std::optional<m2::PointD> m_position;

  bool m_lastUpdate = false;

  // Suggestions language code, not the same as we use in mwm data
  int8_t m_inputLocaleCode = StringUtf8Multilang::kUnsupportedLanguageCode;
  int8_t m_currentLocaleCode = StringUtf8Multilang::kUnsupportedLanguageCode;

  DataSource const & m_dataSource;

  Geocoder::LocalitiesCaches m_localitiesCaches;
  CitiesBoundariesTable m_citiesBoundaries;

  KeywordLangMatcher m_keywordsScorer;
  Emitter m_emitter;
  Ranker m_ranker;
  PreRanker m_preRanker;
  Geocoder m_geocoder;

  bookmarks::Processor m_bookmarksProcessor;

  geo::UnifiedParser m_geoUrlParser;
};
}  // namespace search
