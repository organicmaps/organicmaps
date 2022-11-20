#pragma once

#include "search/cancel_exception.hpp"
#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/cities_boundaries_table.hpp"
#include "search/cuisine_filter.hpp"
#include "search/feature_offset_match.hpp"
#include "search/features_layer.hpp"
#include "search/features_layer_path_finder.hpp"
#include "search/filtering_params.hpp"
#include "search/geocoder_context.hpp"
#include "search/geocoder_locality.hpp"
#include "search/geometry_cache.hpp"
#include "search/mode.hpp"
#include "search/model.hpp"
#include "search/mwm_context.hpp"
#include "search/postcode_points.hpp"
#include "search/query_params.hpp"
#include "search/streets_matcher.hpp"
#include "search/token_range.hpp"
#include "search/tracer.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class CategoriesHolder;
class DataSource;
class MwmValue;

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace search
{
class FeaturesFilter;
class FeaturesLayerMatcher;
class PreRanker;
class TokenSlice;

// This class is used to retrieve all features corresponding to a
// search query.  Search query is represented as a sequence of tokens
// (including synonyms for these tokens), and Geocoder tries to build
// all possible partitions (or layers) of the search query, where each
// layer is a set of features corresponding to some search class
// (e.g. POI, BUILDING, STREET, etc., see search_model.hpp).
// Then, Geocoder builds a layered graph, with edges between features
// on adjacent layers (e.g. between BUILDING ans STREET, STREET and
// CITY, etc.). Usually an edge between two features means that a
// feature from the lowest layer geometrically belongs to a feature
// from the highest layer (BUILDING is located on STREET, STREET is
// located inside CITY, CITY is located inside STATE, etc.). Final
// part is to find all paths through this layered graph and report all
// features from the lowest layer, that are reachable from the
// highest layer.
class Geocoder
{
public:
  struct Params : public QueryParams
  {
    Mode m_mode = Mode::Everywhere;
    m2::RectD m_pivot;
    std::optional<m2::PointD> m_position;
    Locales m_categoryLocales;
    std::vector<uint32_t> m_cuisineTypes;
    std::vector<uint32_t> m_preferredTypes;
    std::shared_ptr<Tracer> m_tracer;

    RecommendedFilteringParams m_filteringParams;

    int m_scale = scales::GetUpperScale();

    bool m_useDebugInfo = false;  // Set to true for debug logs and tests.
  };

  struct LocalitiesCaches
  {
    LocalitiesCaches(base::Cancellable const & cancellable);
    void Clear();

    CountriesCache m_countries;
    StatesCache m_states;
    CitiesTownsOrVillagesCache m_citiesTownsOrVillages;
    VillagesCache m_villages;
  };

  Geocoder(DataSource const & dataSource, storage::CountryInfoGetter const & infoGetter,
           CategoriesHolder const & categories, CitiesBoundariesTable const & citiesBoundaries,
           PreRanker & preRanker, LocalitiesCaches & localitiesCaches,
           base::Cancellable const & cancellable);
  ~Geocoder();

  // Sets search query params.
  void SetParams(Params const & params);

  // Starts geocoding, retrieved features will be appended to
  // |results|.
  void GoEverywhere();
  void GoInViewport();

  // Ends geocoding and informs the following stages
  // of the pipeline (PreRanker and further).
  // This method must be called from the previous stage
  // of the pipeline (the Processor).
  // If |cancelled| is true, the reason for calling Finish must
  // be the cancellation of processing the search request, otherwise
  // the reason must be the normal exit from GoEverywhere or GoInViewport.
  //
  // *NOTE* The caller assumes that a call to this method will never
  // result in search::CancelException even if the shutdown takes
  // noticeable time.
  void Finish(bool cancelled);

  void CacheWorldLocalities();
  void ClearCaches();

private:
  enum class RectId
  {
    Pivot,
    Locality,
    Postcode,
    Suburb,
    Count
  };

  using MwmInfoPtr = std::shared_ptr<MwmInfo>;
  struct ExtendedMwmInfos
  {
    struct ExtendedMwmInfo
    {
      bool operator<(ExtendedMwmInfo const & rhs) const { return m_score < rhs.m_score; }

      MwmInfoPtr m_info;
      MwmContext::MwmType m_type;

      // Score is a rect distance, with exceptions for World, viewport and users's position.
      // Less score is better for search priority.
      double m_score;
    };

    std::vector<ExtendedMwmInfo> m_infos;
    size_t m_firstBatchSize = 0;
  };

  struct Postcodes
  {
    void Clear()
    {
      m_tokenRange.Clear();
      m_countryFeatures.Reset();
      m_worldFeatures.Reset();
    }

    bool Has(uint32_t id, bool searchWorld = false) const
    {
      if (searchWorld)
        return m_worldFeatures.HasBit(id);
      return m_countryFeatures.HasBit(id);
    }

    bool IsEmpty() const { return m_countryFeatures.IsEmpty() && m_worldFeatures.IsEmpty(); }

    TokenRange m_tokenRange;
    CBV m_countryFeatures;
    CBV m_worldFeatures;
  };

  // Sets search query params for categorial search.
  void SetParamsForCategorialSearch(Params const & params);

  void GoImpl(std::vector<MwmInfoPtr> const & infos, bool inViewport);

  template <typename Locality>
  using TokenToLocalities = std::map<TokenRange, std::vector<Locality>>;

  QueryParams::Token const & GetTokens(size_t i) const;

  // Creates a cache of posting lists corresponding to features in m_context
  // for each token and saves it to m_addressFeatures.
  void InitBaseContext(BaseContext & ctx);

  void InitLayer(Model::Type type, TokenRange const & tokenRange, FeaturesLayer & layer);

  void FillLocalityCandidates(BaseContext const & ctx, CBV const & filter,
                              size_t const maxNumLocalities, std::vector<Locality> & preLocalities);

  void FillLocalitiesTable(BaseContext const & ctx);

  void FillVillageLocalities(BaseContext const & ctx);

  bool CityHasPostcode(BaseContext const & ctx) const;

  template <typename Fn>
  void ForEachCountry(ExtendedMwmInfos const & infos, Fn && fn);

  // Throws CancelException if cancelled.
  void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

  // A fast-path branch for categorial requests.
  void MatchCategories(BaseContext & ctx, bool aroundPivot);

  // Tries to find all countries and states in a search query and then
  // performs matching of cities in found maps.
  void MatchRegions(BaseContext & ctx, Region::Type type);

  // Tries to find all cities in a search query and then performs
  // matching of streets in found cities.
  void MatchCities(BaseContext & ctx);

  // Tries to do geocoding without localities, ie. find POIs,
  // BUILDINGs and STREETs without knowledge about country, state,
  // city or village. If during the geocoding too many features are
  // retrieved, viewport is used to throw away excess features.
  void MatchAroundPivot(BaseContext & ctx);

  class CentersFilter
  {
    buffer_vector<m2::PointD, 2> m_centers;
  public:
    void Add(m2::PointD const & pt) { m_centers.push_back(pt); }
    void ProcessStreets(std::vector<uint32_t> & streets, Geocoder & geocoder) const;
  };

  // Tries to do geocoding in a limited scope, assuming that knowledge
  // about high-level features, like cities or countries, is
  // incorporated into |filter|.
  void LimitedSearch(BaseContext & ctx, FeaturesFilter const & filter,
                     CentersFilter const & centers);

  template <typename Fn>
  void WithPostcodes(BaseContext & ctx, Fn && fn);

  // Tries to match some adjacent tokens in the query as streets and
  // then performs geocoding in street vicinities.
  void GreedilyMatchStreets(BaseContext & ctx, CentersFilter const & centers);
  // Matches suburbs and streets inside suburbs like |GreedilyMatchStreets|.
  void GreedilyMatchStreetsWithSuburbs(BaseContext & ctx, CentersFilter const & centers);

  void CreateStreetsLayerAndMatchLowerLayers(BaseContext & ctx,
                                             StreetsMatcher::Prediction const & prediction,
                                             CentersFilter const & centers);

  // Tries to find all paths in a search tree, where each edge is
  // marked with some substring of the query tokens. These paths are
  // called "layer sequence" and current path is stored in |m_layers|.
  void MatchPOIsAndBuildings(BaseContext & ctx, size_t curToken, CBV const & filter);

  // Returns true if current path in the search tree (see comment for
  // MatchPOIsAndBuildings()) looks sane. This method is used as a fast
  // pre-check to cut off unnecessary work.
  bool IsLayerSequenceSane(std::vector<FeaturesLayer> const & layers) const;

  // Finds all paths through layers and emits reachable features from
  // the lowest layer.
  void FindPaths(BaseContext & ctx);

  void TraceResult(Tracer & tracer, BaseContext const & ctx, MwmSet::MwmId const & mwmId,
                   uint32_t ftId, Model::Type type, TokenRange const & tokenRange);

  // Forms result and feeds it to |m_preRanker|.
  void EmitResult(BaseContext & ctx, MwmSet::MwmId const & mwmId, uint32_t ftId, Model::Type type,
                  TokenRange const & tokenRange, IntersectionResult const * geoParts,
                  bool allTokensUsed, bool exactMatch);
  void EmitResult(BaseContext & ctx, Region const & region, TokenRange const & tokenRange,
                  bool allTokensUsed, bool exactMatch);
  void EmitResult(BaseContext & ctx, City const & city, TokenRange const & tokenRange,
                  bool allTokensUsed);

  // Tries to match unclassified objects from lower layers, like
  // parks, forests, lakes, rivers, etc. This method finds all
  // UNCLASSIFIED objects that match to all currently unused tokens.
  void MatchUnclassified(BaseContext & ctx, size_t curToken);

  // A wrapper around RetrievePostcodeFeatures.
  CBV RetrievePostcodeFeatures(MwmContext const & context, TokenSlice const & slice);

  // A caching wrapper around Retrieval::RetrieveGeometryFeatures.
  CBV RetrieveGeometryFeatures(MwmContext const & context, m2::RectD const & rect, RectId id);

  // This is a faster wrapper around SearchModel::GetSearchType(), as
  // it uses pre-loaded lists of streets and villages.
  [[nodiscard]] bool GetTypeInGeocoding(BaseContext const & ctx, uint32_t featureId, Model::Type & type);

  // Reorders maps in a way that prefix consists of "best" maps to search and suffix consists of all
  // other maps ordered by minimum distance from pivot. Returns ExtendedMwmInfos structure which
  // consists of vector of mwms with MwmType information and number of "best" maps to search.
  // For viewport mode prefix consists of maps intersecting with pivot ordered by distance from
  // pivot center.
  // For non-viewport search mode prefix consists of maps intersecting with pivot, map with user
  // location and maps with cities matched to the query, sorting prefers mwms that contain the
  // user's position.
  ExtendedMwmInfos OrderCountries(bool inViewport, std::vector<MwmInfoPtr> const & infos);

  DataSource const & m_dataSource;
  storage::CountryInfoGetter const & m_infoGetter;
  CategoriesHolder const & m_categories;

  StreetsCache m_streetsCache;
  SuburbsCache m_suburbsCache;
  LocalitiesCaches & m_localitiesCaches;
  HotelsCache m_hotelsCache;
  FoodCache m_foodCache;
  cuisine_filter::CuisineFilter m_cuisineFilter;

  base::Cancellable const & m_cancellable;

  // Geocoder params.
  Params m_params;

  // This field is used to map features to a limited number of search
  // classes.
  Model m_model;

  // Following fields are set up by Search() method and can be
  // modified and used only from Search() or its callees.

  MwmSet::MwmId m_worldId;

  // Context of the currently processed mwm.
  std::unique_ptr<MwmContext> m_context;

  // m_cities stores both big cities that are visible at World.mwm
  // and small villages and hamlets that are not.
  TokenToLocalities<City> m_cities;
  TokenToLocalities<Region> m_regions[Region::TYPE_COUNT];
  CitiesBoundariesTable const & m_citiesBoundaries;

  // Caches of features in rects. These caches are separated from
  // TokenToLocalities because the latter are quite lightweight and not
  // all of them are needed.
  PivotRectsCache m_pivotRectsCache;
  PivotRectsCache m_postcodesRectsCache;
  PivotRectsCache m_suburbsRectsCache;
  LocalityRectsCache m_localityRectsCache;

  PostcodePointsCache m_postcodePointsCache;

  // Postcodes features in the mwm that is currently being processed and World.mwm.
  Postcodes m_postcodes;

  // This filter is used to throw away excess features.
  FeaturesFilter const * m_filter;

  // Features matcher for layers intersection.
  std::map<MwmSet::MwmId, std::unique_ptr<FeaturesLayerMatcher>> m_matchersCache;
  FeaturesLayerMatcher * m_matcher;

  // Path finder for interpretations.
  FeaturesLayerPathFinder m_finder;

  // Search query params prepared for retrieval.
  std::vector<SearchTrieRequest<strings::LevenshteinDFA>> m_tokenRequests;
  SearchTrieRequest<strings::PrefixDFAModifier<strings::LevenshteinDFA>> m_prefixTokenRequest;

  ResultTracer m_resultTracer;

  PreRanker & m_preRanker;
};
}  // namespace search
