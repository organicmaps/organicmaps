#pragma once

#include "search/search_query_params.hpp"
#include "search/v2/features_filter.hpp"
#include "search/v2/features_layer.hpp"
#include "search/v2/features_layer_path_finder.hpp"
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
    size_t m_maxNumResults;
  };

  Geocoder(Index & index);

  ~Geocoder() override;

  // Sets search query params.
  void SetParams(Params const & params);

  // Starts geocoding, retrieved features will be appended to
  // |results|.
  void Go(vector<FeatureID> & results);

  void ClearCaches();

private:
  struct Partition
  {
    Partition();

    Partition(Partition &&) = default;

    void FromFeatures(unique_ptr<coding::CompressedBitVector> features,
                      Index::FeaturesLoaderGuard & loader, SearchModel const & model);

    vector<uint32_t> m_clusters[SearchModel::SEARCH_TYPE_COUNT];
    size_t m_size;

    DISALLOW_COPY(Partition);
  };

  // Fills |m_retrievalParams| with [curToken, endToken) subsequence
  // of search query tokens.
  void PrepareRetrievalParams(size_t curToken, size_t endToken);

  // Tries to find all paths in a search tree, where each edge is
  // marked with some substring of the query tokens. These paths are
  // called "layer sequence" and current path is stored in |m_layers|.
  void DoGeocoding(size_t curToken);

  // Returns true if current path in the search tree (see comment for
  // DoGeocoding()) looks sane. This method is used as a fast
  // pre-check to cut off unnecessary work.
  bool IsLayerSequenceSane() const;

  // Finds all paths through layers and emits reachable features from
  // the lowest layer.
  void FindPaths();

  Index & m_index;

  // Geocoder params.
  Params m_params;

  // Total number of search query tokens.
  size_t m_numTokens;

  // This field is used to map features to a limited number of search
  // classes.
  SearchModel const & m_model;

  // Following fields are set up by Search() method and can be
  // modified and used only from Search() or its callees.

  // Value of a current mwm.
  MwmValue * m_value;

  // Id of a current mwm.
  MwmSet::MwmId m_mwmId;

  // Cache of posting lists for each token in the query.  TODO (@y,
  // @m, @vng): consider to update this cache lazily, as user inputs
  // tokens one-by-one.
  vector<Partition> m_partitions;

  // Features loader.
  unique_ptr<Index::FeaturesLoaderGuard> m_loader;

  // Features matcher for layers intersection.
  unique_ptr<FeaturesLayerMatcher> m_matcher;

  // Features filter for interpretations.
  FeaturesFilter m_filter;

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
