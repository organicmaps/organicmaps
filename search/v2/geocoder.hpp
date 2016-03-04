#pragma once

#include "search/cancel_exception.hpp"
#include "search/mode.hpp"
#include "search/search_query_params.hpp"
#include "search/v2/features_layer.hpp"
#include "search/v2/features_layer_path_finder.hpp"
#include "search/v2/mwm_context.hpp"
#include "search/v2/pre_ranking_info.hpp"
#include "search/v2/ranking_utils.hpp"
#include "search/v2/search_model.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/cancellable.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/limits.hpp"
#include "std/set.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

class MwmInfo;
class MwmValue;

namespace coding
{
class CompressedBitVector;
}

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace search
{
namespace v2
{
class FeaturesFilter;
class FeaturesLayerMatcher;
class SearchModel;

// This class is used to retrieve all features corresponding to a
// search query.  Search query is represented as a sequence of tokens
// (including synonyms for these tokens), and Geocoder tries to build
// all possible partitions (or layers) of the search query, where each
// layer is a set of features corresponding to some search class
// (e.g. POI, BUILDING, STREET, etc., see search/v2/search_model.hpp).
// Then, Geocoder builds a layered graph, with edges between features
// on adjacent layers (e.g. between BUILDING ans STREET, STREET and
// CITY, etc.). Usually an edge between two features means that a
// feature from the lowest layer geometrically belongs to a feature
// from the highest layer (BUILDING is located on STREET, STREET is
// located inside CITY, CITY is located inside STATE, etc.). Final
// part is to find all paths through this layered graph and report all
// features from the lowest layer, that are reachable from the
// highest layer.
class Geocoder : public my::Cancellable
{
public:
  struct Params : public SearchQueryParams
  {
    Params();

    Mode m_mode;
    m2::RectD m_viewport;
    /// User's position or viewport center if there is no valid position.
    m2::PointD m_position;
    size_t m_maxNumResults;
  };

  enum RegionType
  {
    REGION_TYPE_STATE,
    REGION_TYPE_COUNTRY,
    REGION_TYPE_COUNT
  };

  struct Locality
  {
    Locality() : m_featureId(0), m_startToken(0), m_endToken(0) {}

    Locality(uint32_t featureId, size_t startToken, size_t endToken)
      : m_featureId(featureId), m_startToken(startToken), m_endToken(endToken)
    {
    }

    MwmSet::MwmId m_countryId;
    uint32_t m_featureId;
    size_t m_startToken;
    size_t m_endToken;
  };

  // This struct represents a country or US- or Canadian- state.  It
  // is used to filter maps before search.
  struct Region : public Locality
  {
    Region(Locality const & l, RegionType type) : Locality(l), m_center(0, 0), m_type(type) {}

    storage::CountryInfoGetter::IdSet m_ids;
    string m_enName;
    m2::PointD m_center;
    RegionType m_type;
  };

  // This struct represents a city or a village. It is used to filter features
  // during search.
  // todo(@m) It works well as is, but consider a new naming scheme
  // when counties etc. are added. E.g., Region for countries and
  // states and Locality for smaller settlements.
  struct City : public Locality
  {
    City(Locality const & l, SearchModel::SearchType type) : Locality(l), m_type(type) {}

    m2::RectD m_rect;
    SearchModel::SearchType m_type;
  };

  using TResult = pair<FeatureID, PreRankingInfo>;
  using TResultList = vector<TResult>;

  Geocoder(Index & index, storage::CountryInfoGetter const & infoGetter);

  ~Geocoder() override;

  // Sets search query params.
  void SetParams(Params const & params);

  // Starts geocoding, retrieved features will be appended to
  // |results|.
  void GoEverywhere(TResultList & results);
  void GoInViewport(TResultList & results);

  void ClearCaches();

private:
  void GoImpl(vector<shared_ptr<MwmInfo>> & infos, bool inViewport);

  template <typename TLocality>
  using TLocalitiesCache = map<pair<size_t, size_t>, vector<TLocality>>;

  enum
  {
    VIEWPORT_ID = -1,
    POSITION_ID = -2,
  };

  SearchQueryParams::TSynonymsVector const & GetTokens(size_t i) const;

  // Fills |m_retrievalParams| with [curToken, endToken) subsequence
  // of search query tokens.
  void PrepareRetrievalParams(size_t curToken, size_t endToken);

  // Creates a cache of posting lists corresponding to features in m_context
  // for each token and saves it to m_addressFeatures.
  void PrepareAddressFeatures();

  void FillLocalityCandidates(coding::CompressedBitVector const * filter,
                              size_t const maxNumLocalities, vector<Locality> & preLocalities);

  void FillLocalitiesTable();

  void FillVillageLocalities();

  template <typename TFn>
  void ForEachCountry(vector<shared_ptr<MwmInfo>> const & infos, TFn && fn);

  // Throws CancelException if cancelled.
  inline void BailIfCancelled()
  {
    ::search::BailIfCancelled(static_cast<my::Cancellable const &>(*this));
  }

  // Tries to find all countries and states in a search query and then
  // performs matching of cities in found maps.
  void MatchRegions(RegionType type);

  // Tries to find all cities in a search query and then performs
  // matching of streets in found cities.
  void MatchCities();

  // Tries to do geocoding without localities, ie. find POIs,
  // BUILDINGs and STREETs without knowledge about country, state,
  // city or village. If during the geocoding too many features are
  // retrieved, viewport is used to throw away excess features.
  void MatchViewportAndPosition();

  // Tries to do geocoding in a limited scope, assuming that knowledge
  // about high-level features, like cities or countries, is
  // incorporated into |filter|.
  void LimitedSearch(FeaturesFilter const & filter);

  // Tries to match some adjacent tokens in the query as streets and
  // then performs geocoding in street vicinities.
  void GreedilyMatchStreets();

  void CreateStreetsLayerAndMatchLowerLayers(
      size_t startToken, size_t endToken, unique_ptr<coding::CompressedBitVector> const & features);

  // Tries to find all paths in a search tree, where each edge is
  // marked with some substring of the query tokens. These paths are
  // called "layer sequence" and current path is stored in |m_layers|.
  void MatchPOIsAndBuildings(size_t curToken);

  // Returns true if current path in the search tree (see comment for
  // MatchPOIsAndBuildings()) looks sane. This method is used as a fast
  // pre-check to cut off unnecessary work.
  bool IsLayerSequenceSane() const;

  // Finds all paths through layers and emits reachable features from
  // the lowest layer.
  void FindPaths();

  // Forms result and appends it to |m_results|.
  void EmitResult(MwmSet::MwmId const & mwmId, uint32_t ftId, SearchModel::SearchType type,
                  size_t startToken, size_t endToken);
  void EmitResult(Region const & region, size_t startToken, size_t endToken);
  void EmitResult(City const & city, size_t startToken, size_t endToken);

  // Computes rank for all results in |m_results|.
  void FillResultRanks();

  // Tries to match unclassified objects from lower layers, like
  // parks, forests, lakes, rivers, etc. This method finds all
  // UNCLASSIFIED objects that match to all currently unused tokens.
  void MatchUnclassified(size_t curToken);

  unique_ptr<coding::CompressedBitVector> LoadCategories(
      MwmContext & context, vector<strings::UniString> const & categories);

  coding::CompressedBitVector const * LoadStreets(MwmContext & context);

  unique_ptr<coding::CompressedBitVector> LoadVillages(MwmContext & context);

  /// A caching wrapper around Retrieval::RetrieveGeometryFeatures.
  /// param[in] Optional query id. Use VIEWPORT_ID, POSITION_ID or feature index for locality.
  coding::CompressedBitVector const * RetrieveGeometryFeatures(
      MwmContext const & context, m2::RectD const & rect, int id);

  // This is a faster wrapper around SearchModel::GetSearchType(), as
  // it uses pre-loaded lists of streets and villages.
  SearchModel::SearchType GetSearchTypeInGeocoding(uint32_t featureId);

  // Returns true iff all tokens are used.
  bool AllTokensUsed() const;

  // Returns true if there exists at least one used token in [from,
  // to).
  bool HasUsedTokensInRange(size_t from, size_t to) const;

  // Counts number of groups of consecutive unused tokens.
  size_t NumUnusedTokensGroups() const;

  // Advances |curToken| to the nearest unused token, or to the end of
  // |m_usedTokens| if there are no unused tokens.
  size_t SkipUsedTokens(size_t curToken) const;

  Index & m_index;

  storage::CountryInfoGetter const & m_infoGetter;

  // Geocoder params.
  Params m_params;

  // Total number of search query tokens.
  size_t m_numTokens;

  // This field is used to map features to a limited number of search
  // classes.
  SearchModel const & m_model;

  // Following fields are set up by Search() method and can be
  // modified and used only from Search() or its callees.

  MwmSet::MwmId m_worldId;

  // Context of the currently processed mwm.
  unique_ptr<MwmContext> m_context;

  // m_cities stores both big cities that are visible at World.mwm
  // and small villages and hamlets that are not.
  TLocalitiesCache<City> m_cities;
  TLocalitiesCache<Region> m_regions[REGION_TYPE_COUNT];

  // Cache of geometry features.
  struct FeaturesInRect
  {
    m2::RectD m_rect;
    unique_ptr<coding::CompressedBitVector> m_cbv;
    int m_id;
  };
  map<MwmSet::MwmId, vector<FeaturesInRect>> m_geometryFeatures;

  // Cache of posting lists for each token in the query.  TODO (@y,
  // @m, @vng): consider to update this cache lazily, as user inputs
  // tokens one-by-one.
  vector<unique_ptr<coding::CompressedBitVector>> m_addressFeatures;

  // Cache of street ids in mwms.
  map<MwmSet::MwmId, unique_ptr<coding::CompressedBitVector>> m_streetsCache;

  // Street features in the mwm that is currently being processed.
  coding::CompressedBitVector const * m_streets;

  // Village features in the mwm that is currently being processed.
  unique_ptr<coding::CompressedBitVector> m_villages;

  // This vector is used to indicate what tokens were matched by
  // locality and can't be re-used during the geocoding process.
  vector<bool> m_usedTokens;

  // This filter is used to throw away excess features.
  FeaturesFilter const * m_filter;

  // Features matcher for layers intersection.
  map<MwmSet::MwmId, unique_ptr<FeaturesLayerMatcher>> m_matchersCache;
  FeaturesLayerMatcher * m_matcher;

  // Path finder for interpretations.
  FeaturesLayerPathFinder m_finder;

  // Search query params prepared for retrieval.
  SearchQueryParams m_retrievalParams;

  // Pointer to the most nested region filled during geocoding.
  Region const * m_lastMatchedRegion;

  // Stack of layers filled during geocoding.
  vector<FeaturesLayer> m_layers;

  // Non-owning pointer to a vector of results.
  TResultList * m_results;
};

string DebugPrint(Geocoder::Locality const & locality);
}  // namespace v2
}  // namespace search
