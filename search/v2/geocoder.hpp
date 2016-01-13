#pragma once

#include "search/cancel_exception.hpp"
#include "search/search_query_params.hpp"
#include "search/v2/features_filter.hpp"
#include "search/v2/features_layer.hpp"
#include "search/v2/features_layer_path_finder.hpp"
#include "search/v2/mwm_context.hpp"
#include "search/v2/search_model.hpp"

#include "indexer/mwm_set.hpp"
#include "indexer/index.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/cancellable.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

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
// features from the lowest layer, that are reachable from the highest
// layer.
class Geocoder : public my::Cancellable
{
public:
  struct Params : public SearchQueryParams
  {
    Params();

    m2::RectD m_viewport;
    m2::PointD m_position;
    size_t m_maxNumResults;
  };

  Geocoder(Index & index, storage::CountryInfoGetter const & infoGetter);

  ~Geocoder() override;

  // Sets search query params.
  void SetParams(Params const & params);

  // Starts geocoding, retrieved features will be appended to
  // |results|.
  void Go(vector<FeatureID> & results);

  void ClearCaches();

private:
  struct Locality
  {
    uint32_t m_featureId = 0;
    size_t m_startToken = 0;
    size_t m_endToken = 0;
  };

  struct Country : public Locality
  {
    Country(Locality const & l) : Locality(l) {}

    vector<size_t> m_regions;
    string m_enName;
  };

  struct City : public Locality
  {
    City(Locality const & l): Locality(l) {}

    m2::RectD m_rect;
  };

  template <typename TLocality>
  using TLocalitiesCache = map<pair<size_t, size_t>, vector<TLocality>>;

  enum { VIEWPORT_ID = -1, POSITION_ID = -2 };

  SearchQueryParams::TSynonymsVector const & GetTokens(size_t i) const;

  // Fills |m_retrievalParams| with [curToken, endToken) subsequence
  // of search query tokens.
  void PrepareRetrievalParams(size_t curToken, size_t endToken);

  void FillLocalitiesTable(MwmContext const & context);

  template <typename TFn>
  void ForEachCountry(vector<shared_ptr<MwmInfo>> const & infos, TFn && fn);

  // Throws CancelException if cancelled.
  inline void BailIfCancelled()
  {
    ::search::BailIfCancelled(static_cast<my::Cancellable const &>(*this));
  }

  // Tries to find all countries in a search query and then performs
  // matching of cities in found countries.
  void MatchCountries();

  // Tries to find all cities in a search query and then performs
  // matching of streets in found cities.
  //
  // *NOTE* that cities will be looked for in a World.mwm, so, for
  // now, villages won't be found on this stage.  TODO (@y, @m, @vng):
  // try to add villages to World.mwm.
  void MatchCities();

  // Tries to do geocoding without localities, ie. find POIs,
  // BUILDINGs and STREETs without knowledge about country, state or
  // city. If during the geocoding too many features are retrieved,
  // viewport is used to throw away excess features.
  void MatchViewportAndPosition();

  // Tries to match some adjacent tokens in the query as streets and
  // then performs geocoding in streets vicinities.
  void GreedilyMatchStreets();

  // Tries to find all paths in a search tree, where each edge is
  // marked with some substring of the query tokens. These paths are
  // called "layer sequence" and current path is stored in |m_layers|.
  void MatchPOIsAndBuildings(size_t curToken);

  // Returns true if current path in the search tree (see comment for
  // DoGeocoding()) looks sane. This method is used as a fast
  // pre-check to cut off unnecessary work.
  bool IsLayerSequenceSane() const;

  // Finds all paths through layers and emits reachable features from
  // the lowest layer.
  void FindPaths();

  coding::CompressedBitVector const * LoadStreets(MwmContext & context);

  /// A caching wrapper around Retrieval::RetrieveGeometryFeatures.
  /// param[in] Optional query id. Use VIEWPORT_ID, POSITION_ID or feature index for locality.
  coding::CompressedBitVector const * RetrieveGeometryFeatures(
      MwmContext const & context, m2::RectD const & rect, int id);

  bool AllTokensUsed() const;

  bool HasUsedTokensInRange(size_t from, size_t to) const;

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

  TLocalitiesCache<City> m_cities;
  TLocalitiesCache<Country> m_countries;

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

  // Streets in a currenly processed mwm.
  coding::CompressedBitVector const * m_streets;

  // This vector is used to indicate what tokens were matched by
  // locality and can't be re-used during the geocoding process.
  vector<bool> m_usedTokens;

  // This filter is used to throw away excess features.
  FeaturesFilter m_filter;

  // Features matcher for layers intersection.
  unique_ptr<FeaturesLayerMatcher> m_matcher;

  // Path finder for interpretations.
  FeaturesLayerPathFinder m_finder;

  // Search query params prepared for retrieval.
  SearchQueryParams m_retrievalParams;

  // Stack of layers filled during geocoding.
  vector<FeaturesLayer> m_layers;

  // Non-owning pointer to a vector of results.
  vector<FeatureID> * m_results;
};
}  // namespace v2
}  // namespace search
